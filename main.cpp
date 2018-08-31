#include <iostream>
#include <thread>

#include "doorbell.h"
#include "udplisten.h"
#include "httpserver.h"

using namespace std;


int main (int argc, char** argv)
{
    cout << "Starting...." << endl;
    boost::asio::io_service io_service;
	DoorBell.Start(&io_service);
    UdpListen.Start(&io_service);
    CHttpServer server(io_service, 12080);

    io_service.run();

	return 0;
}