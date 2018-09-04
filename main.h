#pragma once

class CMain {
public :
    void Start (void);
    void OnInput (const int aGpio, const char aValue);

    boost::asio::io_service mIoService;
};

extern CMain Main;
