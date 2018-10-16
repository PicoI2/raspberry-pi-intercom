#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <boost/bind.hpp>
#include "udp.h"
#include "config.h"

CUdp Udp;

bool CUdp::Start (boost::asio::io_service* apIoService)
{
    mUdpPort = Config.GetULong("udp-port");
    mpIoService = apIoService;
    udp::endpoint endpoint(boost::asio::ip::address::from_string("0.0.0.0"), mUdpPort);
	mpSocket = new udp::socket(*mpIoService, endpoint);
    mpSocket->set_option(udp::socket::reuse_address(true));
    mpSocket->set_option(boost::asio::socket_base::broadcast(true));
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
    cout << "Receive : " << Message << " from : " << mRemoteEndPoint << endl;
    MessageSignal(Message, mRemoteEndPoint);
    StartListening();
}

void CUdp::Send (std::string aMessage)
{
    udp::endpoint endpoint(boost::asio::ip::address::from_string(Config.GetString("server-ip")), mUdpPort);
    mpSocket->send_to(boost::asio::buffer(aMessage), endpoint);
}

void CUdp::SendBroadcast (std::string aMessage)
{
    udp::endpoint endpoint(boost::asio::ip::address::from_string(Config.GetString("broadcast-ip")), mUdpPort);
    mpSocket->send_to(boost::asio::buffer(aMessage), endpoint);
}