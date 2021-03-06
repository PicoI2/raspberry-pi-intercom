#pragma once
#include "audio.h"

#include <boost/asio.hpp>
#include <boost/signals2.hpp>
using namespace std;
using boost::asio::ip::udp;

class CUdp {
public :
    bool Start (boost::asio::io_service* apIoService);
    void Stop ();
    void StartListening (void);
    void ReceiveFrom (const boost::system::error_code& error, size_t bytes_transferred);
    void Send (string aMessage);
    void Send (char* aPacket, size_t aSize);

    boost::signals2::signal <void (const string Message, const udp::endpoint From)> MessageSignal;

    boost::asio::io_service* mpIoService;
    udp::socket* mpSocket;
    array<char, 10*SAMPLE_SIZE> mBuffer;
    udp::endpoint mRemoteEndPoint;
    udp::endpoint mReceiveRemoteEndPoint;
    unsigned long mRemoteUdpPort;
    bool mbValidRemote = false;
};

extern CUdp Udp;
