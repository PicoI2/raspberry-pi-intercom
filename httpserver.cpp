#include "http.h"
#include "httpserver.h"
#include "audio.h"

#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

CHttpServer HttpServer;

// Constructor
bool CHttpServer::Start(boost::asio::io_service* apIoService, int aPort)
{
	// mServer.set_access_channels(websocketpp::log::alevel::all);
    // mServer.clear_access_channels(websocketpp::log::alevel::frame_payload);
	mServer.init_asio(apIoService);
	// mServer.set_reuse_addr(true);
	mServer.set_message_handler([this] (auto hdl, auto message) {this->OnMessage(hdl, message);});
	mServer.set_http_handler([this] (auto hdl) {this->OnHttp(hdl);});
	mServer.listen(aPort);
	mServer.start_accept();

	return true;
}

void CHttpServer::Stop()
{
	mServer.stop_listening();
	// TODO
	// set closing=true, it prevents any new connection by closing it immediately
	// iterate over each connection and send close(conn.second->hdl_, websocketpp::close::status::going_away, "Server shuting down") ;
}


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
        }
    }
	// Or return OK if request has been understood by main
    else {
		RequestSignal(Request);
		// Response = RequestSignal(Request);	// TOTO Make this work
		ConnectionPtr->set_body(Response);
		ConnectionPtr->set_status(websocketpp::http::status_code::ok);
    }
	mServer.start_accept();
}
