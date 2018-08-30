#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include "udplisten.h"

using namespace std;

CUdpListen UdpListen;

bool CUdpListen::Start (void)
{
    mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    cout << "mSocket: " << mSocket << endl;

    sockaddr_in LocalAdress;
    memset((char *) &LocalAdress, 0, sizeof(LocalAdress));
    LocalAdress.sin_family = AF_INET;
    LocalAdress.sin_addr.s_addr = INADDR_ANY;
    LocalAdress.sin_port = htons(12012);
    int ret = bind(mSocket, (sockaddr *) &LocalAdress, sizeof(LocalAdress));
    cout << "bind return: " << ret << endl;
            
	mpThread = new thread(&CUdpListen::Thread, this);
    return true;
}

void CUdpListen::Thread (void)
{
    int ReceiveLen;
    char buffer [256];
    while (1) {
        sockaddr_in Client;
        socklen_t SizeClient = sizeof(Client);
        ReceiveLen = recvfrom(mSocket, &buffer, sizeof(buffer), 0, (struct sockaddr *)&Client, &SizeClient);
        cout << "Receive : " << buffer << endl;

        char address[20];
        inet_ntop(AF_INET, &(Client.sin_addr), address, sizeof(address));
        cout << "From : " << address << endl;
    }
}

void CUdpListen::OpenDoor (void)
{
    
}