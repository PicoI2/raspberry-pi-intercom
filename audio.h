#pragma once

#include <atomic>
#include <mutex>
#include <thread>
#include <queue>

using namespace std;

#define RATE 44100  // Hz
#define DURATION 10 // ms
#define FRAME_BY_SAMPLE (RATE * DURATION / 1000)
#define FRAME_SIZE 2 // 16 bits
#define SAMPLE_SIZE (FRAME_SIZE * FRAME_BY_SAMPLE)    // 882 (16bits at 44.1khz mono)

struct CAudioSample {
    char buf [SAMPLE_SIZE];
};

class CAudio {
public :
    void Play ();
    void Push (CAudioSample* apSample);
    void Record ();
    void Stop ();
protected :
    void PlayThread ();
    thread mPlayThread;
    atomic<bool> mbPlay;
    void RecordThread ();
    thread mRecordThread;
    atomic<bool> mbRecord;
    mutex mMutexQueue;
    queue<CAudioSample*> mSamplesQueue;
};

extern CAudio Audio;
