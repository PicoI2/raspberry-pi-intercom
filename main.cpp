#include <iostream>
// #include <mutex>
#include <thread>
// #include <condition_variable>

#include "doorbell.h"
#include "udplisten.h"

using namespace std;

// mutex mtx;
// condition_variable cv;

int main (int argc, char** argv)
{
	DoorBell.Start();
    UdpListen.Start();

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