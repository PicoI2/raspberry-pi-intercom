#pragma once

#include <set>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

using boost::asio::ip::tcp;

typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
typedef websocketpp::server<websocketpp::config::asio_tls> WSServer;
typedef WSServer::connection_type::request_type WSRequest;
typedef set<websocketpp::connection_hdl,owner_less<websocketpp::connection_hdl> > ConnectionList;

// See https://wiki.mozilla.org/Security/Server_Side_TLS for more details about
// the TLS modes. The code below demonstrates how to implement both the modern
enum tls_mode {
    MOZILLA_INTERMEDIATE = 1,
    MOZILLA_MODERN = 2
};

class CHttpServer
{
public:
	bool Start (boost::asio::io_service* apIoService, int aPort);
	void Stop ();
	void SendMessage (std::string aMessage);
    void SendMessage (char* aMessage, size_t aSize);
	boost::signals2::signal <string (const WSRequest& aHttpRequest)> RequestSignal;

private:
	void OnOpen (websocketpp::connection_hdl hdl);
	void OnMessage (websocketpp::connection_hdl hdl, WSServer::message_ptr msg);
	void OnClose (websocketpp::connection_hdl hdl);
	void OnHttp(websocketpp::connection_hdl hdl);
	void OnTimer (void);

// Members values
	WSServer mServer;
	ConnectionList mClientList;

	boost::asio::io_service* mpIoService;
	boost::asio::deadline_timer* mpTimer;
    boost::posix_time::millisec* mpInterval;
};

extern CHttpServer HttpServer;