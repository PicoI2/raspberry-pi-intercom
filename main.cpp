#include <iostream>

#include "udp.h"
#include "httpserver.h"
#include "io.h"
#include "main.h"
#include "config.h"
#include "pushsafer.h"

using namespace std;
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
        IO.AddOutput(Config.GetULong("output-door-open"), true);
        IO.InputSignal.connect([=](const int aGpio, const bool abValue){
            OnInput(aGpio, abValue);
        });
        HttpServer.Start(&mIoService, stoul(Config.GetString("http-port")));
    }
    else if ("client" == Config.GetString("mode")) {
        cout << "Starting client...." << endl;
    }
    else {
        cerr << "Unknown mode in config file" << endl;
        exit(0);
    }

    Udp.Start(&mIoService);
    mIoService.run();
}

void CMain::OnInput (const int aGpio, const bool abValue) {
    cout << "InputSignal " << aGpio << " " << abValue << endl;
    string Message = "doorbell";
    Udp.SendBroadcast(Message);
    PushSafer.Notification();
}
