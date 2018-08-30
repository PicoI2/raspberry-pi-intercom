#pragma once
#include <thread>
using namespace std;

class CUdpListen {
public :
    bool Start (void);
    void Thread (void);
    void OpenDoor (void);
    thread* mpThread;
    int mSocket;
};

extern CUdpListen UdpListen;
