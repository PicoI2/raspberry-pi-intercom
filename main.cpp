#include <iostream>

#include "doorbell.h"
#include "dooropen.h"
#include "udplisten.h"
#include "httpserver.h"
#include "io.h"

using namespace std;


int main (int argc, char** argv)
{
#ifdef RPI_INTERCOM_SERVER
    cout << "Starting server...." << endl;
#endif
#ifdef RPI_INTERCOM_CLIENT
    cout << "Starting client...." << endl;
#endif
    
    boost::asio::io_service io_service;
#ifdef RPI_INTERCOM_SERVER    
	// DoorBell.Start(&io_service);
    // DoorOpen.Start(&io_service);
    IO.Start(&io_service);
    IO.AddInput(20);
    IO.AddInput(21);
    CHttpServer server(io_service, 12080);
#endif
    UdpListen.Start(&io_service);
    io_service.run();

	return 0;
}