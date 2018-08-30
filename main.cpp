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

	// int ret;
    // cout << "Waiting" << endl;
    // {
    //     std::unique_lock<std::mutex> lck(mtx);
    //     cv.wait(lck);
    // }

    // cout << "End waiting, return " << ret << endl;
    
	DoorBell.mpThread->join();
	return 0;
}