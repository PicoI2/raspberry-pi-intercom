#include <iostream>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

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

	mpThread = new thread(&CDoorBell::Thread, this);
    return true;
}

void CDoorBell::Thread (void)
{
	pthread_setname_np(pthread_self(), "CDoorBell::Thread");
    // Setup poll structure
	struct pollfd polls;
	polls.fd = mFd;
	polls.events = POLLPRI;
	polls.revents = POLLERR | POLLNVAL;

	char c;
	int x = 0;
	while (true) {
		while (x == 0)
		{
			x = poll(&polls, 1, 1000);
			cout << "x:" << x << endl;

            SendNotification();	// TODO REMOVE
		}
		if (x > 0)
		{
            SendNotification();
			lseek(mFd, 0, SEEK_SET);	// Rewind

			read(mFd, &c, 1); // Read & clear
			cout << "c:" << c << endl;
		}
	}
}

void CDoorBell::SendNotification (void)
{
	char message[] = "doorbell\n";
	udp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12012);
	udp::socket socket(*mpIoService, udp::endpoint(udp::v4(), 0));
	socket.send_to(boost::asio::buffer(message, sizeof(message)), endpoint);
}