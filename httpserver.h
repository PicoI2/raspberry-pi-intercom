#pragma once

#include <iostream>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>

using boost::asio::ip::tcp;

typedef websocketpp::server<websocketpp::config::asio> WSServer;
typedef WSServer::connection_type::request_type WSRequest;

class CHttpServer
{
public:
	bool Start (boost::asio::io_service* apIoService, int aPort);
	void Stop ();
	boost::signals2::signal <bool (const WSRequest& aHttpRequest)> RequestSignal;

private:
	void OnMessage(websocketpp::connection_hdl hdl, WSServer::message_ptr msg);
	void OnHttp(websocketpp::connection_hdl hdl);

// Members values
	WSServer mServer;
};

extern CHttpServer HttpServer;