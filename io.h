#pragma once
#include <boost/asio.hpp>
using namespace std;

class CIO {
public :
    bool Start (boost::asio::io_service* apIoService);
    bool AddInput (int aGpio);
    
    void OnTimer (void);
    void SendNotification (int aGpio);
    void WatchInputs (bool abSend);
    
    int mEpoll;
    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    boost::posix_time::millisec* mpInterval;
};

extern CIO IO;
