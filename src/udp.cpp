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
    mpIoService = apIoService;
    udp::endpoint endpoint(boost::asio::ip::address::from_string("0.0.0.0"), Config.GetULong("local-udp-port"));
    mpSocket = new udp::socket(*mpIoService, endpoint);
    mpSocket->set_option(udp::socket::reuse_address(true));

    if ("server" == Config.GetString("mode")) {
        string ClientIP = Config.GetString("client-ip", false);
        if (!ClientIP.empty()) {
            boost::asio::ip::address RemoteAddress = boost::asio::ip::address::from_string(ClientIP);
            mRemoteEndPoint = udp::endpoint(RemoteAddress, Config.GetULong("client-udp-port"));
            mbValidRemote = true;
        }
    }
    else if ("client" == Config.GetString("mode")) {
        boost::asio::ip::address RemoteAddress = boost::asio::ip::address::from_string(Config.GetString("server-ip"));
        mRemoteEndPoint = udp::endpoint(RemoteAddress, Config.GetULong("server-udp-port"));
        mbValidRemote = true;
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
        boost::asio::buffer(mBuffer), mReceiveRemoteEndPoint,
        boost::bind(&CUdp::ReceiveFrom, this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::bytes_transferred));
}

void CUdp::ReceiveFrom (const boost::system::error_code& error, size_t bytes_transferred)
{
    if (SAMPLE_SIZE==bytes_transferred) {
        if (Audio.SetOwner("udp", true)) {
            CAudioSample::Ptr pSample (new CAudioSample());
            memcpy(pSample->buf, mBuffer.data(), SAMPLE_SIZE);
            Audio.Push(pSample);
        }
    }
    else {
        string Message(mBuffer.begin(), mBuffer.begin()+bytes_transferred);
        cout << "Receive : " << Message << " from : " << mReceiveRemoteEndPoint << endl;
        MessageSignal(Message, mReceiveRemoteEndPoint);
    }
    StartListening();
}

void CUdp::Send (string aMessage)
{
    if (mbValidRemote) {
        mpSocket->send_to(boost::asio::buffer(aMessage), mRemoteEndPoint);
    }
}

void CUdp::Send (char* aPacket, size_t aSize)
{
    if (mbValidRemote) {
        mpSocket->send_to(boost::asio::buffer(aPacket, aSize), mRemoteEndPoint);
    }
}
