#pragma once

#include <iostream>

#include <boost/asio.hpp>

#include "httpconnection.h"

using boost::asio::ip::tcp;

class CHttpServer
{
public:
	CHttpServer(boost::asio::io_service& aIoService, int aPort);

private:
	void StartAccept();
	void HandleAccept(CHttpConnection::pointer aNewConnection, const boost::system::error_code& error);

// Members values
	boost::asio::io_service& mIoService;
	tcp::acceptor mAcceptor;
};
