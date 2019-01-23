#include <iostream>
#include <thread>
#include <jsoncpp/json/json.h>

#include "udp.h"
#include "http.h"
#include "httpserver.h"
#include "io.h"
#include "main.h"
#include "config.h"
#include "pushsafer.h"
#include "ring.h"
#include "audio.h"
#include "sessions.h"

CMain Main;

int main (int argc, char** argv)
{
    char* pNameConfigFile = nullptr;
    if (1 < argc && Config.ReadConfigFile(argv[1])) {
        Main.Start();
    }
    else {
        cerr << "Missing config file" << endl;
    }

    return 0;
}

void CMain::Start ()
{
    bool bRes = true;

    cout << "Starting..." << endl;
    mbClientMode = ("client" == Config.GetString("mode"));

    // Catch Ctrl-C
    boost::asio::signal_set ExitSignal (mIoService, SIGINT);
    ExitSignal.async_wait([=](const boost::system::error_code& error, int signal_number){
        OnExit(error, signal_number);
    });

    // Start UDP
    Udp.Start(&mIoService);
    Udp.MessageSignal.connect([=](const string Message, const udp::endpoint From){
        OnMessage(Message);
    });

    IO.Start(&mIoService);
    if (!mbClientMode) {    // If server
        PushSafer.Init(&mIoService);
        
        mInputRingBell = Config.GetULong("input-ringbell");
        IO.AddInput(mInputRingBell);

        mOutputDoorOpen = Config.GetULong("output-door-open", false);
        if (mOutputDoorOpen) {
            IO.AddOutput(mOutputDoorOpen, false);
        }
    }
    else {  // If client, you could use buttons if you don't have a screen
        mInputStopRing = Config.GetULong("input-stop-ring", false);
        if (mInputStopRing) {
            IO.AddInput(mInputStopRing);
        }

        mInputListen = Config.GetULong("input-listen", false);
        if (mInputListen) {
            IO.AddInput(mInputListen);
        }

        mInputSpeak = Config.GetULong("input-speak", false);
        if (mInputSpeak) {
            IO.AddInput(mInputSpeak);
        }

        mInputOpenDoor = Config.GetULong("input-open-door", false);
        if (mInputOpenDoor) {
            IO.AddInput(mInputOpenDoor);
        }

        mInputHangup = Config.GetULong("input-hangup", false);
        if (mInputHangup) {
            IO.AddInput(mInputHangup);
        }
    }
    IO.InputSignal.connect([=](const int aGpio, const bool abValue){
        OnInput(aGpio, abValue);
    });
    
    // Start HttpServer
    HttpServer.Start(&mIoService, Config.GetULong("http-port", false));
    HttpServer.RequestSignal.connect([=](const WSRequest& aHttpRequest){
        return OnRequest(aHttpRequest);
    });

    // Init Audio
    Audio.Init();
    Ring.Init(&mIoService);

    // Start boost
    cout << "Run boost service" << endl;
    mIoService.run();
}

// When an input change
void CMain::OnInput (const int aGpio, const bool abValue) {
    cout << "InputSignal " << aGpio << " " << abValue << endl;
    if (!mbClientMode) {    // If server
        if (mInputRingBell == aGpio) {
            string Message = "doorbell";
            Ring.Start();
            Udp.Send(Message);
            HttpServer.SendMessage(Message);
            PushSafer.Notification("Someone is at your door");
        }
    }
    else {  // If client
        if (mInputStopRing == aGpio) {
            Ring.Stop();
        }
        else if (mInputListen == aGpio) {
            Ring.Stop();
            Udp.Send("record");
            Audio.Play();
        }
        else if (mInputSpeak == aGpio) {
            Ring.Stop();
            Udp.Send("play");
            Audio.Play();
            Audio.Record();
        }
        else if (mInputOpenDoor == aGpio) {
            Ring.Stop();
            Udp.Send("dooropen");
        }
        else if (mInputHangup == aGpio) {
            Ring.Stop();
            Udp.Send("hangup");
            Audio.Stop();
        }
    }
}

// When an UDP message is received
void CMain::OnMessage (const string aMessage) {
    if (!mbClientMode) {    // If server
        if (string::npos != aMessage.find("record")) {
            Audio.Record();
            Ring.Stop();
        }
        else if (string::npos != aMessage.find("play")) {
            Audio.Record();
            Audio.Play();
            Ring.Stop();
        }
        else if (string::npos != aMessage.find("hangup")) {
            Audio.Stop();
            Audio.SetOwner("udp", false);
        }
        else if (string::npos != aMessage.find("dooropen")) {
            IO.SetOutput(mOutputDoorOpen, true, 2000);
        }
        else {
            cout << "Received unknown message :'" << aMessage << "'" << endl;
        }
    }
    else {  // If client
        if (string::npos != aMessage.find("doorbell")) {
            Ring.Start();
        }
        else {
            cout << "Received unknown message :'" << aMessage << "'" << endl;
        }
    }
}

// When an HTTP Request does not concern a file
string CMain::OnRequest (const WSRequest& aHttpRequest) {
    bool bPasswordOk = Config.GetString("password", false).empty();
    string SessionId = http::GetCookie(aHttpRequest.get_header("Cookie"), "session_id");
    bPasswordOk = bPasswordOk || Sessions.IsSessionExist(SessionId);
    string Result;
    
    if (aHttpRequest.get_method() == http::method::GET) {
        if ("/config" == aHttpRequest.get_uri()) {
            // Return config in json
            Json::Value JsonConfig;
            JsonConfig["mode"] = Config.GetString("mode");
            JsonConfig["frameBySample"] = FRAME_BY_SAMPLE;
            JsonConfig["rate"] = RATE;
            JsonConfig["videoSrc"] = mbClientMode ? Config.GetString("server-ip") : "";
            Result = JsonConfig.toStyledString();
        }
        else if ("/password_ok" == aHttpRequest.get_uri()) {
            Result = bPasswordOk ? "true" : "false";
        }
        else if ("/audiobusy" == aHttpRequest.get_uri()) {
            Result = Audio.GetOwner().empty() ? "false" : "true";
        }
        else if (!bPasswordOk) {
            Result = "Wrong password";
        }
        // All case below this line works only if bPasswordOk
        else if (!mbClientMode) {    // If server
            if ("/startlisten" == aHttpRequest.get_uri()) {
                Audio.Record();
                Result = "OK";
            }
            else if ("/startspeaking" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Audio.Record();
                Audio.Play();
                Result = "OK";
            }
            else if ("/dooropen" == aHttpRequest.get_uri()) {
                IO.SetOutput(mOutputDoorOpen, true, 2000);
                Ring.Stop();
                Udp.Send("dooropen");
                Result = "OK";
            }
            else if ("/hangup" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Audio.Stop();
                Audio.SetOwner(SessionId, false);
                Result = "OK";
            }
        }
        else {  // If client
            if ("/stopring" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Result = "OK";
            }
            else if ("/startlisten" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Udp.Send("record");
                Audio.Play();
                Result = "OK";
            }
            else if ("/startspeaking" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Udp.Send("play");
                Audio.Play();
                Audio.Record();
                Result = "OK";
            }
            else if ("/dooropen" == aHttpRequest.get_uri()) {
                Ring.Stop();
                Udp.Send("dooropen");
                Result = "OK";
            }
            else if ("/hangup" == aHttpRequest.get_uri()) {
                Udp.Send("hangup");
                Ring.Stop();
                Audio.Stop();
                Result = "OK";
            }
        }
    }
    return Result;
}

// When Ctrl-C
void CMain::OnExit (const boost::system::error_code& error, int signal_number)
{
    cout << " Exit on signal " << signal_number << endl;
    Ring.Stop();
    HttpServer.Stop();
    IO.Stop();
    Udp.Stop();
    Audio.Stop();
    exit(0);
}
