#include "audio.h"
#include "udp.h"
#include "httpserver.h"
#include "config.h"
#include "io.h"

#include <stdio.h>
#include <iostream>

CAudio Audio;

// Initialization
void CAudio::Init()
{
    mNbAudioUser = 0;
    mOutputAudioOn = Config.GetULong("output-audio-on", false);
    if (mOutputAudioOn) {
        IO.AddOutput(mOutputAudioOn, false);
    }

    mEchoState = speex_echo_state_init(FRAME_SIZE, SAMPLE_SIZE);
}

// Set audio output on/off
void CAudio::AudioOnOff (bool abOn)
{
    if (mOutputAudioOn) {
        if (abOn) {
            ++mNbAudioUser;
        }
        else {
            --mNbAudioUser;
        }
        assert (mNbAudioUser >= 0 && mNbAudioUser <= 2);
        IO.SetOutput(mOutputAudioOn, mNbAudioUser > 0);
    }
}

// Request playback. The worker thread opens the PCM and starts playing.
void CAudio::Play()
{
    if (!mbPlay) {
        mbPlay = true;
        StartThread();
    }
}

// Push an audio sample in play queue
void CAudio::Push (CAudioSample::Ptr apSample) {
    if (mbPlay) {
        mMutexQueue.lock();
        // If more than 100 samples in queue (~4.6 seconds), delete the older one.
        if (mSamplesQueue.size() > 100) {
            mSamplesQueue.pop();
            cerr << "mSamplesQueue is full" << endl;
        }
        mSamplesQueue.push(apSample);
        mMutexQueue.unlock();
    }
}

// Open and configure a PCM handle. Returns true on success.
// Called from the worker thread only.
bool CAudio::OpenPcm(bool bRecord)
{
    // Keep the config string alive: GetString returns by value, so calling
    // .c_str() on the temporary directly would leave a dangling pointer.
    string CardName = Config.GetString(bRecord ? "sound-card-rec" : "sound-card-play");
    const char* name = CardName.empty() ? "default" : CardName.c_str();

    int err;

    /* Open the PCM device */
    snd_pcm_t* PcmHandle = nullptr;
    if ((err = snd_pcm_open(&PcmHandle, name, bRecord ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        cerr << "ERROR: Can't open " << name << " PCM device (" << snd_strerror (err) << ")" << endl;
        return false;
    }

    /* Allocate parameters object and fill it with default values*/
    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(PcmHandle, params);

    /* Set parameters */
    if ((err = snd_pcm_hw_params_set_access(PcmHandle, params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
        cerr << "ERROR: Can't set interleaved mode (" << snd_strerror (err) << ")" << endl;
    }

    if ((err = snd_pcm_hw_params_set_format(PcmHandle, params, SND_PCM_FORMAT_S16_LE)) < 0) {
        cerr << "ERROR: Can't set format (" << snd_strerror (err) << ")" << endl;
    }

    if ((err = snd_pcm_hw_params_set_channels(PcmHandle, params, 1)) < 0) {
        cerr << "ERROR: Can't set channels number (" << snd_strerror (err) << ")" << endl;
    }
    unsigned int rate = RATE;
    if ((err = snd_pcm_hw_params_set_rate_near(PcmHandle, params, &rate, 0)) < 0) {
        cerr << "ERROR: Can't set rate (" << snd_strerror (err) << ")" << endl;
    }

    cout << "snd_pcm_hw_params_set_rate_near to:" << rate << endl;

    /* Write parameters */
    if ((err = snd_pcm_hw_params(PcmHandle, params)) < 0) {
        cerr << "ERROR: Can't set hardware parameters (" << snd_strerror (err) << ")" << endl;
        snd_pcm_close(PcmHandle);
        return false;
    }

    // Publish the handle under lock so Stop() can safely drop it to unblock us
    lock_guard<mutex> Lock(mMutexPcm);
    if (bRecord) {
        mRecordPcmHandle = PcmHandle;
    }
    else {
        mPlayPcmHandle = PcmHandle;
    }
    return true;
}

// Drop and close a PCM handle. Called from the worker thread only.
void CAudio::ClosePcm(bool bRecord)
{
    cout << (bRecord ? "Stop recording" : "Stop playing") << endl;

    // Hold mMutexPcm for the whole drop+close so it cannot race with Stop()
    // calling snd_pcm_drop() on the same handle.
    {
        lock_guard<mutex> Lock(mMutexPcm);
        snd_pcm_t* PcmHandle = bRecord ? mRecordPcmHandle : mPlayPcmHandle;
        if (!PcmHandle) {
            return;
        }

        int err;
        // Drop (not drain): drop discards buffered samples and never blocks.
        if ((err = snd_pcm_drop(PcmHandle)) < 0) {
            cerr << "ERROR: snd_pcm_drop (" << snd_strerror (err) << ")" << endl;
        }
        if ((err = snd_pcm_close(PcmHandle)) < 0) {
            cerr << "ERROR: snd_pcm_close (" << snd_strerror (err) << ")" << endl;
        }

        if (bRecord) {
            mRecordPcmHandle = nullptr;
        }
        else {
            mPlayPcmHandle = nullptr;
        }
    }

    if (!bRecord) {
        AudioOnOff(false);
    }
}

void CAudio::StartThread() {
    if (mbThreadRunning) {
        return; // A worker thread is already running
    }
    // A previous worker may have exited on its own (e.g. on a PCM error) and
    // still be joinable: reap it before starting a new one, otherwise the
    // "joinable" state would wrongly prevent any restart.
    if (mThread.joinable()) {
        mThread.join();
    }
    mbThreadRunning = true;
    mThread = thread ([this](){
        Thread();
    });
}

// Play and record thread.
// Owns the PCM handles: it opens/closes them to match the desired mbPlay /
// mbRecord state and recovers from (or tears down) per-stream errors on its own.
void CAudio::Thread()
{
    int err;
    CAudioSample::Ptr pPlaySample;

    cout << "Start Thread" << endl;

    while (mbPlay || mbRecord) {
        // Reconcile open PCMs with the desired state
        if (mbPlay && !mPlayPcmHandle) {
            if (OpenPcm(false)) {
                AudioOnOff(true);
            }
            else {
                mbPlay = false;     // Could not open playback, give up this stream
            }
        }
        else if (!mbPlay && mPlayPcmHandle) {
            ClosePcm(false);
        }
        if (mbRecord && !mRecordPcmHandle) {
            if (!OpenPcm(true)) {
                mbRecord = false;   // Could not open capture, give up this stream
            }
        }
        else if (!mbRecord && mRecordPcmHandle) {
            ClosePcm(true);
        }

        if (mbPlay && mPlayPcmHandle) {
            if (mSamplesQueue.empty() && !mbRecord) {
                // Wait for samples
                this_thread::sleep_for(chrono::milliseconds(1));
            }
            else if (!mSamplesQueue.empty()) {
                // Play sample
                mMutexQueue.lock();
                pPlaySample = mSamplesQueue.front();
                mSamplesQueue.pop();
                mMutexQueue.unlock();

                err = snd_pcm_writei(mPlayPcmHandle, pPlaySample->buf, FRAME_BY_SAMPLE);
                if (err == -EPIPE) {
                    cout << "XRUN " << endl;
                    this_thread::sleep_for(chrono::milliseconds(80));
                    snd_pcm_prepare(mPlayPcmHandle);
                }
                else if (err < 0) {
                    cerr << "ERROR. Can't write to PCM device (" << snd_strerror(err) << ")" << endl;
                    mbPlay = false;     // Tear down playback only; recording keeps going
                }
            }
        }

        if (mbRecord && mRecordPcmHandle) {
            CAudioSample::Ptr pSample (new CAudioSample());
            err = snd_pcm_readi (mRecordPcmHandle, pSample->buf, FRAME_BY_SAMPLE);
            if (err == -EPIPE) {
                cerr << "OVERRUN on capture" << endl;
                snd_pcm_prepare(mRecordPcmHandle);  // Recoverable, keep recording
            }
            else if (err < 0) {
                cerr << "read from audio interface failed (" << snd_strerror (err) << ")" << endl;
                mbRecord = false;   // Tear down recording only; playback keeps going
            }
            else if (err == FRAME_BY_SAMPLE) {
                if (pPlaySample) {
                    CAudioSample::Ptr pSampleWithoutEcho (new CAudioSample());
                    speex_echo_cancellation(mEchoState, (spx_int16_t*)pSample->buf, (spx_int16_t*)pPlaySample->buf, (spx_int16_t*)pSampleWithoutEcho->buf);
                    pSample = pSampleWithoutEcho;
                }

                HttpServer.SendMessage(pSample->buf, SAMPLE_SIZE);
                Udp.Send(pSample->buf, SAMPLE_SIZE);
            }
            // else: short read, skip this sample
        }
    }

    // Loop ended (both streams stopped or errored): release any handle still open.
    if (mPlayPcmHandle) {
        ClosePcm(false);
    }
    if (mRecordPcmHandle) {
        ClosePcm(true);
    }

    mbThreadRunning = false;
}

// Request recording. The worker thread opens the PCM and starts capturing.
void CAudio::Record()
{
    if (!mbRecord) {
        mbRecord = true;
        StartThread();
    }
}

// Stop audio thread
void CAudio::Stop()
{
    cout << "Audio stop..." << endl;
    mbPlay = false;
    mbRecord = false;

    // Unblock the worker thread before joining it: a blocking snd_pcm_writei /
    // snd_pcm_readi will NOT return just because the flags above changed, so we
    // drop the PCM(s) to force any in-progress call to return. Without this the
    // join() below could block for a long time and freeze the io_service thread.
    // The lock serializes with the worker's own open/close of these handles.
    {
        lock_guard<mutex> Lock(mMutexPcm);
        if (mPlayPcmHandle) {
            snd_pcm_drop(mPlayPcmHandle);
        }
        if (mRecordPcmHandle) {
            snd_pcm_drop(mRecordPcmHandle);
        }
    }

    if (mThread.joinable()) {
        mThread.join();
    }
    // The worker closes the PCM handles itself on the way out.

    mMutexQueue.lock();
    while (!mSamplesQueue.empty()) {
        mSamplesQueue.pop();
    }
    mMutexQueue.unlock();
    cout << "Audio stopped" << endl;
}

// Set owner for audio from distant to local speaker.
// This is to manage case where 2 user answer in the same time
// Return true if succeed, false if an user owns already audio canal
bool CAudio::SetOwner (string aOwner, bool bWantToOwn)
{
    if (mOwner.empty() && bWantToOwn) {
        mOwner = aOwner;
        HttpServer.SendMessage("audiobusy");
    }
    bool bRes = (aOwner == mOwner);
    if (bRes && !bWantToOwn) {
        mOwner.clear();
        HttpServer.SendMessage("audiofree");
    }
    return bRes;
}
