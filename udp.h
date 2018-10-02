#pragma once
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
    void SendBroadcast (std::string aMessage);

    boost::signals2::signal <void (const std::string Message, const udp::endpoint From)> MessageSignal;

    boost::asio::io_service* mpIoService;
    udp::socket* mpSocket;
    std::array<char, 255> mBuffer;
    udp::endpoint mRemoteEndPoint;
    unsigned long mUdpPort;
};

extern CUdp Udp;
