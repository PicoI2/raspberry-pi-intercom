#pragma once

#include <set>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/signals2.hpp>
#include <websocketpp/server.hpp>
using boost::asio::ip::tcp;

#ifdef USE_HTTPS
#include <websocketpp/config/asio.hpp>
typedef websocketpp::server<websocketpp::config::asio_tls> WSServer;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;
#else
#include <websocketpp/config/asio_no_tls.hpp>
typedef websocketpp::server<websocketpp::config::asio> WSServer;
#endif

typedef WSServer::connection_type::request_type WSRequest;
typedef set<websocketpp::connection_hdl,owner_less<websocketpp::connection_hdl> > ConnectionList;

class CHttpServer
{
public:
    bool Start (boost::asio::io_service* apIoService, int aPort);
    void Stop ();
    void SendMessage (string aMessage);
    void SendMessage (const char* aMessage, size_t aSize);
    boost::signals2::signal <string (const WSRequest& aHttpRequest)> RequestSignal;

private:
    void OnOpen (websocketpp::connection_hdl hdl);
    void OnMessage (websocketpp::connection_hdl hdl, WSServer::message_ptr msg);
    void OnClose (websocketpp::connection_hdl hdl);
    void OnHttp(websocketpp::connection_hdl hdl);
    void OnTimer (void);
    #ifdef USE_HTTPS
    context_ptr OnTlsInit (websocketpp::connection_hdl hdl);
    #endif

// Members values
    WSServer mServer;
    ConnectionList mClientList;

    boost::asio::io_service* mpIoService;
    boost::asio::deadline_timer* mpTimer;
    boost::posix_time::millisec* mpInterval;

    string mPassword;
};

extern CHttpServer HttpServer;