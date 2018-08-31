#include "httpconnection.h"
#include "httprequest.h"
#include "requesthandler.h"
#include "http.h"

const long TIMEOUT = 60; // seconds

// Constructor
CHttpConnection::CHttpConnection(boost::asio::io_service& aIoService) 
    : mSocket(aIoService)
    , mTimer(aIoService)
{
}

// Init connection (start listening)
void CHttpConnection::Init ()
{
    mClientAsString = mSocket.remote_endpoint().address().to_string() + ":" + std::to_string(mSocket.remote_endpoint().port());

    // Configure timeout
    ResetTimeout();
    
    StartListening();
}

// Start listening from client
void CHttpConnection::StartListening ()
{
    // Start listening for message.
    mSocket.async_read_some(boost::asio::buffer(mBuffer), 
        [self = shared_from_this()]	(auto error, size_t bytes_transferred){self->HandleRead(error, bytes_transferred);}
    );

}

// Callback on data received from client
void CHttpConnection::HandleRead(const boost::system::error_code& error, size_t bytes_transferred)
{
    bool bCloseConnexion = error;
    if (!error)
    {
        // Relauch timeout timer
        ResetTimeout();

        std::string message(mBuffer.begin(), mBuffer.begin() + bytes_transferred);

        // Display received message
        // std::cout << "\033[1;36m" << message << "\033[0m";

        // Append to request
        mRequest.append(message);

        // Limit email size
        if (MAX_REQUEST_SIZE < mRequest.size())
        {
            bCloseConnexion = true;
        }
        else if (std::string::npos != mRequest.find("\r\n\r\n"))
        {
            CHttpRequest Request(mRequest);
            CRequestHandler Handler(Request);
            Send(Handler.Response());
        }
    }

    if (error)
    {
        // std::cout << "ERROR: " << error.message() << std::endl;
    }

    if (bCloseConnexion)
    {
        Send(http::code::BAD_REQUEST);
    }
    else
    {
        // Start listening for message again
        StartListening();
    }
}

// Send message to client
void CHttpConnection::Send (std::string aMessage)
{
    if (mSocket.is_open())
    {
        // Uncomment to see server response
        // cout << "\033[1;31m" << aMessage << "\033[0m";
        replyBuffers.push_back(boost::asio::buffer(aMessage));
        boost::asio::async_write(mSocket, replyBuffers,
            [self = shared_from_this()] (auto error, auto bytes_transferred) {self->HandleWrite(error, bytes_transferred);}
        );
    }
}

// Callback at each write on socket
void CHttpConnection::HandleWrite(const boost::system::error_code& error, size_t bytes_transferred)
{
    if (error)
    {
        std::cout << error.message() << std::endl;
    }
    CloseConnection();
}

// Reset timeout to initial value
void CHttpConnection::ResetTimeout ()
{
    mTimer.expires_from_now(boost::posix_time::seconds(TIMEOUT));
    // Changing expiration time cancel the timer, so we have to start a new asynchronous wait
    mTimer.async_wait(
        [self = shared_from_this()] (const boost::system::error_code& error_code)
        {
            if (boost::asio::error::operation_aborted != error_code)
            {
                std::cout << "Connection timeout" << std::endl;
                self->CloseConnection();
            }
        }
    );
}

// Close connection
void CHttpConnection::CloseConnection()
{
    mTimer.cancel();
    if (mSocket.is_open())
    {
        std::cout << "Close client connection with " << mClientAsString << std::endl;
        mSocket.close();
    }
}
