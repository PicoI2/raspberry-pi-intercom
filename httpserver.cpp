#include "httpserver.h"

using namespace std;

CHttpServer HttpServer;

// Constructor
bool CHttpServer::Start(boost::asio::io_service* apIoService, int aPort)
{
	mpIoService = apIoService;
	mpAcceptor = new tcp::acceptor(*mpIoService, tcp::endpoint(tcp::v4(), aPort));
	StartAccept();
	return true;
}

void CHttpServer::Stop()
{
	// TODO
}

// Start or restart listen on server socket
void CHttpServer::StartAccept()
{
	CHttpConnection::pointer NewConnection = CHttpConnection::pointer(new CHttpConnection (*mpIoService));

	mpAcceptor->async_accept(NewConnection->socket(),
		[this, NewConnection] (auto error) {this->HandleAccept(NewConnection, error);}
	);

	cout << "Listening " << mpAcceptor->local_endpoint() << endl;
}

// Callback on new client connection			
void CHttpServer::HandleAccept(CHttpConnection::pointer aNewConnection, const boost::system::error_code& error)
{
	if (!error)	{
		aNewConnection->Init();
		cout << "Connection of client: " << aNewConnection->ToString() << endl;
    }
    else {
        cerr << error.message() << endl;
	}
	StartAccept();	// Accept next client
}