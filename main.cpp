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

    // Catch Ctrl-C
    boost::asio::signal_set ExitSignal (mIoService, SIGINT);
    ExitSignal.async_wait([=](const boost::system::error_code& error, int signal_number){
        OnExit(error, signal_number);
    });

    // Start HttpServer
    HttpServer.Start(&mIoService, stoul(Config.GetString("http-port")));
    HttpServer.RequestSignal.connect([=](const CHttpRequest& aHttpRequest){
        return OnRequest(aHttpRequest);
    });

    // Start UDP
    Udp.Start(&mIoService);
    Udp.MessageSignal.connect([=](const std::string Message, const udp::endpoint From){
        OnMessage(Message);
    });

    IO.Start(&mIoService);
    if ("server" == Config.GetString("mode")) {
        cout << "Starting server...." << endl;
        PushSafer.Init(&mIoService);
        
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
        });
    }
    else if ("client" == Config.GetString("mode")) {
        cout << "Starting client...." << endl;
    }
    else {
        cerr << "Unknown mode in config file" << endl;
        exit(0);
    }

    // Start boost
    mIoService.run();
}

// When an input change
void CMain::OnInput (const int aGpio, const bool abValue) {
    cout << "InputSignal " << aGpio << " " << abValue << endl;
    string Message = "doorbell";
    Udp.Send(Message);
    PushSafer.Notification("Someone is at your door");
}

// When an UDP message is received
void CMain::OnMessage (const string aMessage) {
    if (string::npos != aMessage.find("doorbell")) {
        Audio.Ring();
    }
    else {
        cout << "Received unknown message :'" << aMessage << "'" << endl;
    }
}

// When an HTTP Request does not concern a file
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

// When Ctrl-C
void CMain::OnExit (const boost::system::error_code& error, int signal_number)
{
    cout << " Exit on signal " << signal_number << endl;
    Audio.Stop();
    HttpServer.Stop();
    IO.Stop();
    Udp.Stop();
    exit(0);
}
