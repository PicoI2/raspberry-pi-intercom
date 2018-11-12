#pragma once

class CMain {
public:
    void Start ();
private:
    void OnInput (const int aGpio, const bool abValue);
    void OnMessage (const string aMessage);
    string OnRequest (const WSRequest& aHttpRequest);
    void OnExit (const boost::system::error_code& error, int signal_number);

    boost::asio::io_service mIoService;
    bool mbClientMode = false;

    // Server IO
    long mInputRingBell;
    long mOutputDoorOpen;

    // Client IO
    long mInputStopRing;
    long mInputListen;
    long mInputSpeak;
    long mInputOpenDoor;
    long mInputHangup;
};

extern CMain Main;
