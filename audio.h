#pragma once

#include <atomic>
#include <thread>

using namespace std;

class CAudio {
public :
    void Ring ();
    void Stop ();
protected :
    void Thread ();
    thread mThread;
    atomic<bool> mbPlaying;
};

extern CAudio Audio;
