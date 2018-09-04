#include <iostream>

#include "udp.h"
#include "httpserver.h"
#include "io.h"
#include "main.h"

using namespace std;
CMain Main;

int main (int argc, char** argv)
{
    Main.Start();
	return 0;
}

void CMain::Start (void)
{
#ifdef RPI_INTERCOM_SERVER
    cout << "Starting server...." << endl;
#endif
#ifdef RPI_INTERCOM_CLIENT
    cout << "Starting client...." << endl;
#endif
    
#ifdef RPI_INTERCOM_SERVER    
    IO.Start(&mIoService);
    IO.AddInput(20);
    IO.AddInput(21);
    IO.AddOutput(26, false);
    IO.InputSignal.connect([=](const int aGpio, const char aValue){
        OnInput(aGpio, aValue);
    });
    CHttpServer server(mIoService, 12080);
#endif
    Udp.Start(&mIoService);
    OnInput(1, '1');
    mIoService.run();
}

void CMain::OnInput (const int aGpio, const char aValue) {
    // TEST
    cout << "InputSignal " << aGpio << " " << aValue << endl;
    string Message = string("InputSignal ") + to_string(aGpio) + " " + aValue;
    Udp.Send(Message);
    IO.SetOutput(26, true, 5000);
    // TEST
}
