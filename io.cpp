#include <iostream>
#include <fstream>
#include <sys/epoll.h>

#include <boost/bind.hpp>

#include "io.h"

using namespace std;

CIO IO;

bool CIO::Start (boost::asio::io_service* apIoService)
{
    mEpoll = epoll_create(1);   // Size ignored
    mpIoService = apIoService;
    mpInterval = new boost::posix_time::millisec(40);
    mpTimer = new boost::asio::deadline_timer(*mpIoService, *mpInterval);
    mpTimer->async_wait(boost::bind(&CIO::OnTimer, this));
    // mpTimer->async_wait([this]{Thread();});
    
    return true;
}

bool CIO::AddInput (int aGpio)
{
    bool bRes = false;
    ofstream ExportFile("/sys/class/gpio/export");
    if (ExportFile.is_open()) {
        ExportFile << aGpio;
        ExportFile.close();

        string EdgeFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/edge";
        ofstream EdgeFile(EdgeFileName);
        if (EdgeFile.is_open()) {
            EdgeFile << "both";
            EdgeFile.close();

            string ValueFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/value";
            int ValueFile = open(ValueFileName.c_str(), O_RDWR | O_NONBLOCK);
            if (ValueFile > 0) {
                epoll_event pollEvent;
                pollEvent.events = EPOLLPRI;
                pollEvent.data.u64 = (uint64_t)aGpio << 32;   // Hidding GPIO number in u64 value. Be careful, data is an union !
                pollEvent.data.fd = ValueFile;

                if (0 == epoll_ctl(mEpoll, EPOLL_CTL_ADD, ValueFile, &pollEvent)) {
                    WatchInputs(false);
                    bRes = true;
                }
            }
        }
    }
    cout << "AddInput " << aGpio << " return " << bRes << endl;
    return bRes;
}

void CIO::WatchInputs (bool abSend)
{
    epoll_event Event;
    while (0 < epoll_wait(mEpoll, &Event, 1, 0)) {
        char Value;
        lseek(Event.data.fd, 0, SEEK_SET);
        read(Event.data.fd, &Value, 1);
        int Gpio = (Event.data.u64>>32);
        cout << "Event on input " << Gpio << " value: " << Value << endl;
        if (abSend) {
            SendNotification(Gpio);
        }
    }
}

void CIO::OnTimer (void)
{
    WatchInputs(true);
    
    // Reschedule the timer in the future:
    mpTimer->expires_from_now(*mpInterval);
    mpTimer->async_wait(boost::bind(&CIO::OnTimer, this));
}

void CIO::SendNotification (int aGpio)
{
    // TODO
}