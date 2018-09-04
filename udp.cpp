#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <boost/bind.hpp>
#include "udplisten.h"

CUdpListen UdpListen;

bool CUdpListen::Start (boost::asio::io_service* apIoService)
{
    udp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 12012);
	mpSocket = new udp::socket(*apIoService, endpoint);
	StartListening();
    return true;
}

void CUdpListen::StartListening (void)
{
    mpSocket->async_receive_from(
        boost::asio::buffer(mBuffer), mRemoteEndPoint,
        boost::bind(&CUdpListen::ReceiveFrom, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void CUdpListen::ReceiveFrom (const boost::system::error_code& error, std::size_t bytes_transferred)
{
    std::string message(mBuffer.begin(), mBuffer.begin()+bytes_transferred);
    cout << "Receive : " << message << "From : " << mRemoteEndPoint << endl;
    StartListening();
}

void CUdpListen::OpenDoor (void)
{
    
}