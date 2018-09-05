#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <boost/bind.hpp>
#include "udp.h"

CUdp Udp;

bool CUdp::Start (boost::asio::io_service* apIoService)
{
    mpIoService = apIoService;
    udp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12012);
	mpSocket = new udp::socket(*mpIoService, endpoint);
	StartListening();
    return true;
}

void CUdp::StartListening (void)
{
    mpSocket->async_receive_from(
        boost::asio::buffer(mBuffer), mRemoteEndPoint,
        boost::bind(&CUdp::ReceiveFrom, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void CUdp::ReceiveFrom (const boost::system::error_code& error, std::size_t bytes_transferred)
{
    std::string Message(mBuffer.begin(), mBuffer.begin()+bytes_transferred);
    cout << "Receive : " << Message << "From : " << mRemoteEndPoint << endl;
    MessageSignal(Message, mRemoteEndPoint);
    StartListening();
}

void CUdp::Send (std::string aMessage)
{
    udp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12012);
    udp::socket socket(*mpIoService, udp::endpoint(udp::v4(), 0));
    socket.send_to(boost::asio::buffer(aMessage), endpoint);
}

void CUdp::SendBroadcast (std::string aMessage)
{
    udp::endpoint endpoint(boost::asio::ip::address::from_string("192.168.10.255"), 12012);
    udp::socket socket(*mpIoService, udp::v4());
    socket.set_option(udp::socket::reuse_address(true));
    socket.set_option(boost::asio::socket_base::broadcast(true));
    socket.send_to(boost::asio::buffer(aMessage), endpoint);
    socket.close();
}