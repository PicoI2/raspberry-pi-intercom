#pragma once
#include <thread>
#include <boost/asio.hpp>
using namespace std;

class CDoorBell {
public :
    bool Start (boost::asio::io_service* apIoService);
    void Thread (void);
    void SendNotification (void);
    thread* mpThread;
    int mFd;
    boost::asio::io_service* mpIoService;
};

extern CDoorBell DoorBell;
