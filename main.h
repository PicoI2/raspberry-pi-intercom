#pragma once

class CMain {
public :
    void Start ();
    void OnInput (const int aGpio, const bool abValue);
    void OnMessage (const string aMessage);
    bool OnRequest (const WSRequest& aHttpRequest);
    void OnExit (const boost::system::error_code& error, int signal_number);

    boost::asio::io_service mIoService;
};

extern CMain Main;
