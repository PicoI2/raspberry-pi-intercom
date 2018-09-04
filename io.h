#pragma once
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
using namespace std;

class CIO {
public :
    bool Start (boost::asio::io_service* apIoService);
    bool AddInput (int aGpio);
    
    void OnTimer (void);
    void WatchInputs (bool abSendSignal);

    boost::signals2::signal <void (const int aGpio, const char aValue)> InputSignal;
    
    int mEpoll;
    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    boost::posix_time::millisec* mpInterval;
};

extern CIO IO;
