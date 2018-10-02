#pragma once

#include <atomic>

using namespace std;

class CAudio {
public :
    void Ring ();
    void Stop ();
protected :
    void Thread ();

    atomic<bool> mbPlaying;
};

extern CAudio Audio;
