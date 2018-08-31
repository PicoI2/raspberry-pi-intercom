#include <iostream>
#include <fstream>
#include <boost/bind.hpp>

#include "dooropen.h"

using namespace std;

CDoorOpen DoorOpen;

bool CDoorOpen::Start (boost::asio::io_service* apIoService)
{
    mpIoService = apIoService;
    mpInterval = new boost::posix_time::seconds(1);
    mpTimer = new boost::asio::deadline_timer(*mpIoService, *mpInterval);

    return true;
}

bool CDoorOpen::Open (void)
{
    bool bRes = Write('1');

    mpTimer->expires_from_now(*mpInterval);
    mpTimer->async_wait(boost::bind(&CDoorOpen::EndTimer, this));
    // mpTimer->async_wait([this]{EndTimer();});
    

    return bRes;
}

void CDoorOpen::EndTimer (void)
{
    Write('0');
}

bool CDoorOpen::Write (char c)
{
    bool bRes = false;
    ofstream OutputFile("value");
    if (OutputFile.is_open()) {
        cout << "Writing " << c << endl;
        OutputFile << c;
        OutputFile.close();
        bRes = true;
    }
    else {
        cout << "Error writing to " << "value" << endl;
    }
    return bRes;
}