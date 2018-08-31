#pragma once

#include <iostream>
#include <memory>	// enable_shared_from_this
#include <array>

#include <boost/asio.hpp>

using boost::asio::ip::tcp;

static constexpr size_t MAX_REQUEST_SIZE = 1024*1024; // 1 Mo

class CHttpConnection : public std::enable_shared_from_this<CHttpConnection>
{
public:
	typedef std::shared_ptr<CHttpConnection> pointer;

	// Constructor
	CHttpConnection(boost::asio::io_service& aIoService);

	void Send (std::string aMessage);
	void Init ();

	tcp::socket& socket() {return mSocket;}
	const std::string ToString() {return mClientAsString;}

private:	
	void HandleWrite(const boost::system::error_code& error, size_t bytes_transferred);
	void StartListening ();
	void HandleRead(const boost::system::error_code& error, size_t bytes_transferred);
	void CloseConnection();
	void ResetTimeout ();

private:
	boost::asio::deadline_timer mTimer;	// Timeout
	tcp::socket mSocket;
	std::array<char, MAX_REQUEST_SIZE> mBuffer;
	std::string mClientAsString;	// somthing like 127.0.0.1:45677
	std::string mRequest;		// Complete request
};