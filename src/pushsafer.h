#pragma once
#include <boost/asio.hpp>

using namespace std;
using boost::asio::ip::tcp;

class CPushSafer
{
public:
    bool Init (boost::asio::io_service* apIoService);
    bool Notification (string aMessage);
    string ToFormData (string aKey, string aValue);

    boost::asio::io_service* mpIoService;
};

extern CPushSafer PushSafer;