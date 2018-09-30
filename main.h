#pragma once

class CMain {
public :
    void Start ();
    void OnInput (const int aGpio, const bool abValue);
    void OnMessage (const string aMessage);

    boost::asio::io_service mIoService;
};

extern CMain Main;
