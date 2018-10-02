#include <iostream>
#include <thread>

#include "udp.h"
#include "http.h"
#include "httpserver.h"
#include "io.h"
#include "main.h"
#include "config.h"
#include "pushsafer.h"
#include "audio.h"

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
    if ("server" == Config.GetString("mode")) {
        cout << "Starting server...." << endl;
        PushSafer.Init(&mIoService);
        IO.Start(&mIoService);
        IO.AddInput(Config.GetULong("input-ringbell"));
        unsigned long OutputDoorOpen = Config.GetULong("output-door-open", false);
        if (OutputDoorOpen) {
            IO.AddOutput(Config.GetULong("output-door-open"), true);
        }
        IO.InputSignal.connect([=](const int aGpio, const bool abValue){
            OnInput(aGpio, abValue);
        });
        thread ([](){
            system("./pjsua --config-file pjsua-server.conf");
            cerr << "Pjsua exit" << endl;
            PushSafer.Notification("Error: rpi-intercom server stoped");
            terminate();
        }).detach();
    }
    else if ("client" == Config.GetString("mode")) {
        cout << "Starting client...." << endl;
    }
    else {
        cerr << "Unknown mode in config file" << endl;
        exit(0);
    }

    HttpServer.Start(&mIoService, stoul(Config.GetString("http-port")));
    HttpServer.RequestSignal.connect([=](const CHttpRequest& aHttpRequest){
        return OnRequest(aHttpRequest);
    });

    Udp.Start(&mIoService);
    Udp.MessageSignal.connect([=](const std::string Message, const udp::endpoint From){
        OnMessage(Message);
    });
    mIoService.run();
}

void CMain::OnInput (const int aGpio, const bool abValue) {
    cout << "InputSignal " << aGpio << " " << abValue << endl;
    string Message = "doorbell";
    Udp.SendBroadcast(Message);
    PushSafer.Notification("Someone is at your door");
}

void CMain::OnMessage (const string aMessage) {
    if (string::npos != aMessage.find("doorbell")) {
        Audio.Ring();
    }
    else {
        cout << "Received unknown message :'" << aMessage << "'" << endl;
    }
}

bool CMain::OnRequest (const CHttpRequest& aHttpRequest) {
    bool bRes;
    if (aHttpRequest.mMethod == http::method::PUT) {
        if ("/dooropen" == aHttpRequest.mUri) {
            if (IO.SetOutput(26, true, 2000))
            {
                bRes = true;
            }
        }
        else if ("/stopring" == aHttpRequest.mUri) {
            Audio.Stop();
            bRes = true;
        }
    }
    return bRes;
}