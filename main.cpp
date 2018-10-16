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
    Main.Start();
	return 0;
}

void CMain::Start (void)
{
    Config.SetParameter("port-udp", "12012");
    PushSafer.Init(&mIoService);
#ifdef RPI_INTERCOM_SERVER
    cout << "Starting server...." << endl;
    Config.ReadConfigFile("server.cfg");
#endif
#ifdef RPI_INTERCOM_CLIENT
    cout << "Starting client...." << endl;
    Config.ReadConfigFile("client.cfg");
#endif
    
#ifdef RPI_INTERCOM_SERVER    
    IO.Start(&mIoService);
    IO.AddInput(20);
    IO.AddInput(21);
    IO.AddOutput(26, true);
    IO.InputSignal.connect([=](const int aGpio, const bool abValue){
        OnInput(aGpio, abValue);
    });
    CHttpServer server(mIoService, 12080);
#endif
    Udp.Start(&mIoService);
    mIoService.run();
}

void CMain::OnInput (const int aGpio, const bool abValue) {
    cout << "InputSignal " << aGpio << " " << abValue << endl;
    string Message = "doorbell";
    Udp.SendBroadcast(Message);
    string Message2 = "nobroadcast";
    Udp.Send(Message2);
    PushSafer.Notification();
}
