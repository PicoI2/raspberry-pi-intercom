#pragma once

#include <iostream>

#include <boost/asio.hpp>

#include "httpconnection.h"

using boost::asio::ip::tcp;

class CHttpServer
{
public:
	bool Start(boost::asio::io_service* mpIoService, int aPort);

private:
	void StartAccept();
	void HandleAccept(CHttpConnection::pointer aNewConnection, const boost::system::error_code& error);

// Members values
	boost::asio::io_service* mpIoService;
	tcp::acceptor* mpAcceptor;
};

extern CHttpServer HttpServer;