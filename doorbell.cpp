#include <iostream>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include <boost/bind.hpp>

#include "doorbell.h"
#include "udplisten.h"

using namespace std;
using boost::asio::ip::udp;

CDoorBell DoorBell;

bool CDoorBell::Start (boost::asio::io_service* apIoService)
{
    mpIoService = apIoService;
    mFd = open ("value" , O_RDWR);

    char c;
    size_t count;
    if (ioctl (mFd, FIONREAD, &count) != -1)
    for (int i = 0 ; i < count ; i++) // Clear any initial pending interrupt
        read(mFd, &c, 1); // Catch value to suppress compiler unused warning

    // Setup poll structure
    mPolls.fd = mFd;
    mPolls.events = POLLPRI;
    mPolls.revents = POLLERR | POLLNVAL;

    mpInterval = new boost::posix_time::millisec(1000);
    mpTimer = new boost::asio::deadline_timer(*mpIoService, *mpInterval);
    mpTimer->async_wait(boost::bind(&CDoorBell::Thread, this));
    // mpTimer->async_wait([this]{Thread();});
    
    return true;
}

void CDoorBell::Thread (void)
{
    char c;
    cout << "poll..." << endl;
    int x = poll(&mPolls, 1, 0);
    cout << "x:" << x << endl;

    SendNotification();	// TODO REMOVE
    if (x > 0)
    {
        SendNotification();
        lseek(mFd, 0, SEEK_SET);	// Rewind

        read(mFd, &c, 1); // Read & clear
        cout << "c:" << c << endl;
    }

    // Reschedule the timer in the future:
    //mpTimer->expires_at(mpTimer->expires_at() + *mpInterval);
    mpTimer->expires_from_now(*mpInterval);
    mpTimer->async_wait(boost::bind(&CDoorBell::Thread, this));
}

void CDoorBell::SendNotification (void)
{
    char message[] = "doorbell\n";
    udp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12012);
    udp::socket socket(*mpIoService, udp::endpoint(udp::v4(), 0));
    socket.send_to(boost::asio::buffer(message, sizeof(message)), endpoint);
}