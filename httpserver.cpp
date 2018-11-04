#include "http.h"
#include "httpserver.h"
#include "audio.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

CHttpServer HttpServer;

const static string ALIVE  = "alive";

context_ptr OnTlsInit(websocketpp::connection_hdl hdl);

// Constructor
bool CHttpServer::Start(boost::asio::io_service* apIoService, int aPort)
{
	mpIoService = apIoService;

	//mServer.set_access_channels(websocketpp::log::alevel::all);
    mServer.clear_access_channels(websocketpp::log::alevel::all);
	mServer.init_asio(mpIoService);
	mServer.set_reuse_addr(true);
	mServer.set_open_handler([this] (auto hdl) {this->OnOpen(hdl);});
	mServer.set_close_handler([this] (auto hdl) {this->OnClose(hdl);});
	mServer.set_message_handler([this] (auto hdl, auto message) {this->OnMessage(hdl, message);});
	mServer.set_http_handler([this] (auto hdl) {this->OnHttp(hdl);});
	mServer.set_tls_init_handler(bind(&OnTlsInit,::_1));
	mServer.listen(aPort);
	mServer.start_accept();

	mpInterval = new boost::posix_time::millisec(30000); // 30 seconds
    mpTimer = new boost::asio::deadline_timer(*mpIoService, *mpInterval);
    mpTimer->async_wait([this](const boost::system::error_code&){OnTimer();});

	return true;
}

// Stop the seever and close all weboscket
void CHttpServer::Stop()
{
	mServer.stop_listening();
	for (auto hdl : mClientList) {
		mServer.close(hdl, websocketpp::close::status::going_away, "Server shuting down");
	}
}

// When a client open a websocket
void CHttpServer::OnOpen (websocketpp::connection_hdl hdl) {
	mServer.send(hdl, ALIVE, websocketpp::frame::opcode::text);
	mClientList.insert(hdl);
}

// When a client close a websocket
void CHttpServer::OnClose (websocketpp::connection_hdl hdl) {
	mClientList.erase(hdl);
}

// When a client send us a message
void CHttpServer::OnMessage(websocketpp::connection_hdl hdl, WSServer::message_ptr msg)
{
	string Message = msg->get_payload();
	cout << "OnMessage, message length: " << Message.length() << endl;

	for (size_t i=0; i<Message.length()/SAMPLE_SIZE; ++i) {
		CAudioSample::Ptr pSample (new CAudioSample());
		memcpy(pSample->buf, Message.data()+(i*SAMPLE_SIZE), SAMPLE_SIZE);
		Audio.Push(pSample);	
	}
}

// Broadcast text message to all clients
void CHttpServer::SendMessage (std::string aMessage) {
	for (auto hdl : mClientList) {
		mServer.send(hdl, aMessage, websocketpp::frame::opcode::text);
	}
}

// Broadcast binary message to all clients
void CHttpServer::SendMessage (char* aMessage, size_t aSize) {
	for (auto hdl : mClientList) {
		mServer.send(hdl, aMessage, aSize, websocketpp::frame::opcode::binary);
	}
}

// On HTTP request
void CHttpServer::OnHttp(websocketpp::connection_hdl hdl)
{
	WSServer::connection_ptr ConnectionPtr = mServer.get_con_from_hdl(hdl);

	// Read parsed request
	WSRequest Request = ConnectionPtr->get_request();
    string Method = Request.get_method();
    string Uri = Request.get_uri();
    string MimeType = http::mime::GetMimeType(Uri);

	// To redirect GET '/' to GET '/index.html'
	if (MimeType.empty() && "GET" == Method && "/" == Uri) {
		Uri = "/index.html";
		MimeType = http::mime::HTML;
	}
	
	string Response;
	bool bOk = false;
	// Return requested file
	if (Method == http::method::GET && !MimeType.empty() && string::npos == Uri.find("..")) {
        ifstream RequestedFile("./www/" + Uri, std::ios::in | std::ios::binary);
        if (RequestedFile.is_open()) {
            char buffer [1024];
            while (RequestedFile.read(buffer, sizeof(buffer)).gcount() > 0) {
                Response.append(buffer, RequestedFile.gcount());
            }
			ConnectionPtr->set_body(Response);
    		ConnectionPtr->set_status(websocketpp::http::status_code::ok);
			ConnectionPtr->append_header("Content-Type", MimeType);
			bOk = true;
        }
    }
	// Or return OK if request has been understood by main
    if (!bOk) {
		auto res = RequestSignal(Request);
		if (res) {
			ConnectionPtr->set_body(res.get());
			ConnectionPtr->set_status(websocketpp::http::status_code::ok);
			ConnectionPtr->append_header("Content-Type", http::mime::HTML);
			bOk = true;
		}
    }
	if (!bOk) {
		ConnectionPtr->set_status(websocketpp::http::status_code::bad_request);
	}
	mServer.start_accept();
}

void CHttpServer::OnTimer (void)
{
	SendMessage(ALIVE);
	// for (websocketpp::connection_hdl hdl : mClientList) {
		// TODO Disconnect socket if no message have been received for more than 60 seconds, after having added life message from client to server
	// }

	mpTimer->expires_from_now(*mpInterval);
	mpTimer->async_wait([this](const boost::system::error_code&){OnTimer();});
}

std::string get_password() {
    return "rpi-intercom";
}

context_ptr OnTlsInit (websocketpp::connection_hdl hdl) {

	tls_mode mode = MOZILLA_INTERMEDIATE;

	namespace asio = websocketpp::lib::asio;

    std::cout << "on_tls_init called with hdl: " << hdl.lock().get() << std::endl;
    std::cout << "using TLS mode: " << (mode == MOZILLA_MODERN ? "Mozilla Modern" : "Mozilla Intermediate") << std::endl;

    context_ptr ctx = websocketpp::lib::make_shared<asio::ssl::context>(asio::ssl::context::sslv23);

    try {
        if (mode == MOZILLA_MODERN) {
            // Modern disables TLSv1
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::no_tlsv1 |
                             asio::ssl::context::single_dh_use);
        } else {
            ctx->set_options(asio::ssl::context::default_workarounds |
                             asio::ssl::context::no_sslv2 |
                             asio::ssl::context::no_sslv3 |
                             asio::ssl::context::single_dh_use);
        }
        ctx->set_password_callback(bind(&get_password));
        ctx->use_certificate_chain_file("ssl/cert.pem");
        ctx->use_private_key_file("ssl/key.pem", asio::ssl::context::pem);
        ctx->use_tmp_dh_file("ssl/dh.pem");
        
        std::string ciphers;
        
        if (mode == MOZILLA_MODERN) {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
        } else {
            ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
        }
        
        if (SSL_CTX_set_cipher_list(ctx->native_handle() , ciphers.c_str()) != 1) {
            std::cout << "Error setting cipher list" << std::endl;
        }
    } catch (std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
	return ctx;
}