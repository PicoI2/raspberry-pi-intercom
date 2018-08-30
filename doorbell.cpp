#include <iostream>
#include <poll.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "doorbell.h"
#include "udplisten.h"

// #include <mutex>
// #include <condition_variable>

using namespace std;

// extern mutex mtx;
// extern condition_variable cv;

CDoorBell DoorBell;

bool CDoorBell::Start (void)
{
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

            // std::unique_lock<std::mutex> lck(mtx);
            // cv.notify_one();
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
    sockaddr_in DestAdress;
    memset((char *) &DestAdress, 0, sizeof(DestAdress));
    DestAdress.sin_family = AF_INET;
    DestAdress.sin_port = htons(12012);
    DestAdress.sin_addr.s_addr = INADDR_ANY;
    inet_aton("127.0.0.1", &DestAdress.sin_addr);
    
    char buffer[] = "doorbell\n";
    sendto(UdpListen.mSocket, &buffer, sizeof(buffer), 0, (struct sockaddr *)&DestAdress, sizeof(DestAdress));
}