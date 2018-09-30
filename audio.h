#pragma once

#include <atomic>

using namespace std;

class CAudio {
public :
    void Ring  ();
    void Thread ();
protected :
    atomic<bool> mbPlaying;
};

extern CAudio Audio;
