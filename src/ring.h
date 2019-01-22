#pragma once

#include <atomic>
#include <thread>
#include <boost/asio.hpp>

using namespace std;

class CRing {
public :
    void Init (boost::asio::io_service* apIoService);
    void Start ();
    void Stop ();
protected :
    void Thread ();
    void OnTimer ();
    thread mThread;
    atomic<bool> mbPlaying;
    boost::posix_time::millisec* mpInterval;
    boost::asio::deadline_timer* mpTimer;
};

extern CRing Ring;
