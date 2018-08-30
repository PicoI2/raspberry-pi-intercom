#include <iostream>
#include <thread>

#include "doorbell.h"
#include "udplisten.h"

using namespace std;


int main (int argc, char** argv)
{
    cout << "Starting...." << endl;
    boost::asio::io_service io_service;
	DoorBell.Start(&io_service);
    UdpListen.Start(&io_service);

    io_service.run();

	return 0;
}