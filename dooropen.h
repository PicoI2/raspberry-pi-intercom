#pragma once
#include <boost/asio.hpp>
using namespace std;

class CDoorOpen {
public :
    bool Start (boost::asio::io_service* apIoService);
    bool Open (void);
    void EndTimer (void);
    bool Write (char c);

    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    boost::posix_time::seconds* mpInterval;
};

extern CDoorOpen DoorOpen;


