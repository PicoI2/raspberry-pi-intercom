#pragma once
#include <thread>
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::udp;

class CUdpListen {
public :
    bool Start (boost::asio::io_service* apIoService);
    void StartListening (void);
    void ReceiveFrom (const boost::system::error_code& error, std::size_t bytes_transferred);
    void OpenDoor (void);
    udp::socket* mpSocket;
    std::array<char, 255> mBuffer;
    udp::endpoint mRemoteEndPoint;
};

extern CUdpListen UdpListen;
