#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include "httpconnection.h"
#include "httprequest.h"

using boost::asio::ip::tcp;

class CHttpServer
{
public:
	bool Start(boost::asio::io_service* mpIoService, int aPort);

	boost::signals2::signal <bool (const CHttpRequest& aHttpRequest)> RequestSignal;

private:
	void StartAccept();
	void HandleAccept(CHttpConnection::pointer aNewConnection, const boost::system::error_code& error);


// Members values
	boost::asio::io_service* mpIoService;
	tcp::acceptor* mpAcceptor;
};

extern CHttpServer HttpServer;