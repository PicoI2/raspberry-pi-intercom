#pragma once
#include <thread>
using namespace std;

class CDoorBell {
public :
    bool Start (void);
    void Thread (void);
    void SendNotification (void);
    thread* mpThread;
    int mFd;
};

extern CDoorBell DoorBell;
