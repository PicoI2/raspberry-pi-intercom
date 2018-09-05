#pragma once
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
using namespace std;

class CIO {
public :
    class CInput {
    public:
        CInput(int aFd, int aGpio, bool abValue, boost::asio::io_service* apIoService)
            :mFd(aFd)
            ,mGpio(aGpio)
            ,mbValue(abValue)
            ,mTimer(*apIoService)
        {}
        int mFd;
        int mGpio;
        bool mbValue;
        boost::asio::deadline_timer mTimer;
        bool mbDebouncing = false;
    };
    
    bool Start (boost::asio::io_service* apIoService);
    bool AddInput (int aGpio);
    bool AddOutput (int aGpio, bool abValue);
    bool SetOutput (int aGpio, bool abValue, int aDurationMs=0);
    
    void OnTimer (void);
    void WatchInputs (bool abSendSignal);
    bool ReadValue (int aFd);
    void OnDebounceEnd (CInput* pInput);

    boost::signals2::signal <void (const int aGpio, const bool aValue)> InputSignal;
    
    int mEpoll;
    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    boost::posix_time::millisec* mpInterval;
    boost::posix_time::millisec* mpDebounce;

    map<int, CInput*> InputsMap;
};

extern CIO IO;
