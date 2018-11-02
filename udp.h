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
    void ReceiveFrom (const boost::system::error_code& error, std::size_t bytes_transferred);
    void Send (std::string aMessage);
    void Send (char* aPacket, size_t aSize);

    boost::signals2::signal <void (const std::string Message, const udp::endpoint From)> MessageSignal;

    boost::asio::io_service* mpIoService;
    udp::socket* mpSocket;
    std::array<char, 10*SAMPLE_SIZE> mBuffer;
    udp::endpoint mRemoteEndPoint;
    udp::endpoint mReceiveRemoteEndPoint;
    unsigned long mRemoteUdpPort;
};

extern CUdp Udp;
