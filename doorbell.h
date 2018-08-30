#pragma once
#include <thread>
#include <boost/asio.hpp>
using namespace std;

class CDoorBell {
public :
    bool Start (boost::asio::io_service* apIoService);
    void Thread (void);
    void SendNotification (void);
    int mFd;
    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    struct pollfd mPolls;
    boost::posix_time::millisec* mpInterval;
};

extern CDoorBell DoorBell;
