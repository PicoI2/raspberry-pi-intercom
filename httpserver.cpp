#include "httpserver.h"

// Constructor
CHttpServer::CHttpServer(boost::asio::io_service& aIoService, int aPort)
	: mIoService(aIoService)
	, mAcceptor(aIoService, tcp::endpoint(tcp::v4(), aPort))
{
	StartAccept();
}
			
// Start or restart listen on server socket
void CHttpServer::StartAccept()
{
	CHttpConnection::pointer NewConnection = CHttpConnection::pointer(new CHttpConnection (mIoService));
				
	mAcceptor.async_accept(NewConnection->socket(),
		[this, NewConnection] (auto error) {this->HandleAccept(NewConnection, error);}
	);

	std::cout << "Listening " << mAcceptor.local_endpoint() << std::endl;
}

// Callback on new client connection			
void CHttpServer::HandleAccept(CHttpConnection::pointer aNewConnection, const boost::system::error_code& error)
{
	if (!error)
	{
		aNewConnection->Init();
		std::cout << "Connection of client: " << aNewConnection->ToString() << std::endl;
	}
	StartAccept();	// Accept next client
}