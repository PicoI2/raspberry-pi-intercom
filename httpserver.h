#pragma once

#include <set>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <websocketpp/config/debug_asio_no_tls.hpp>

using boost::asio::ip::tcp;

typedef websocketpp::server<websocketpp::config::asio> WSServer;
typedef WSServer::connection_type::request_type WSRequest;
typedef set<websocketpp::connection_hdl,owner_less<websocketpp::connection_hdl> > ConnectionList;

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