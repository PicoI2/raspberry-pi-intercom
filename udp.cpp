#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <boost/bind.hpp>
#include "udp.h"
#include "config.h"
#include "audio.h"

CUdp Udp;

bool CUdp::Start (boost::asio::io_service* apIoService)
{
    mUdpPort = Config.GetULong("udp-port");
    mpIoService = apIoService;
    udp::endpoint endpoint(boost::asio::ip::address::from_string("0.0.0.0"), mUdpPort);
	mpSocket = new udp::socket(*mpIoService, endpoint);
    mpSocket->set_option(udp::socket::reuse_address(true));

    if ("server" == Config.GetString("mode")) {
        mRemoteAddress = boost::asio::ip::address::from_string(Config.GetString("client-ip"));
    }
    else if ("client" == Config.GetString("mode")) {
        mRemoteAddress = boost::asio::ip::address::from_string(Config.GetString("server-ip"));
    }
    
	StartListening();
    return true;
}

void CUdp::Stop()
{
	mpSocket->close();
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
    if (SAMPLE_SIZE==bytes_transferred) {
        CAudioSample* pSample = new CAudioSample();
        memcpy(pSample->buf, mBuffer.data(), SAMPLE_SIZE);
        Audio.Push(pSample);
    }
    else {
        std::string Message(mBuffer.begin(), mBuffer.begin()+bytes_transferred);
        cout << "Receive : " << Message << " from : " << mRemoteEndPoint << endl;
        MessageSignal(Message, mRemoteEndPoint);
    }
    StartListening();
}

void CUdp::Send (std::string aMessage)
{
    udp::endpoint endpoint(mRemoteAddress, mUdpPort);
    mpSocket->send_to(boost::asio::buffer(aMessage), endpoint);
}

void CUdp::Send (char* aPacket, size_t aSize)
{
    udp::endpoint endpoint(mRemoteAddress, mUdpPort);
    mpSocket->send_to(boost::asio::buffer(aPacket, aSize), endpoint);
}
