#pragma once

#include <atomic>
#include <thread>

using namespace std;

class CRing {
public :
    void Start ();
    void Stop ();
protected :
    void Thread ();
    thread mThread;
    atomic<bool> mbPlaying;
};

extern CRing Ring;
