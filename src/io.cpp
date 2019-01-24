#include <iostream>
#include <fstream>
#include <sys/epoll.h>
#include <chrono>
#include <thread>

#include "io.h"

using namespace std;

CIO IO;

bool CIO::Start (boost::asio::io_service* apIoService)
{
    mEpoll = epoll_create(1);   // Size ignored
    mpIoService = apIoService;
    mpInterval = new boost::posix_time::millisec(40);
    mpDebounce = new boost::posix_time::millisec(100);
    mpTimer = new boost::asio::deadline_timer(*mpIoService, *mpInterval);
    mpTimer->async_wait([this](const boost::system::error_code& error){
        if (!error) {
            OnTimer();
        }
    });

    return true;
}

void CIO::Stop ()
{
    mpTimer->cancel();
}

bool CIO::AddInput (int aGpio)
{
    bool bRes = false;
    ofstream ExportFile("/sys/class/gpio/export");
    if (ExportFile.is_open()) {
        ExportFile << aGpio;
        ExportFile.close();
        // Must wait after export eiher /sys/class/gpio/gpioXX does not have right permission
        this_thread::sleep_for(chrono::milliseconds(100));

        string EdgeFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/edge";
        ofstream EdgeFile(EdgeFileName);
        if (EdgeFile.is_open()) {
            EdgeFile << "both";
            EdgeFile.close();

            string ValueFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/value";
            int ValueFile = open(ValueFileName.c_str(), O_RDWR | O_NONBLOCK);
            if (ValueFile > 0) {
                epoll_event pollEvent;
                pollEvent.events = EPOLLPRI;
                pollEvent.data.fd = ValueFile;

                InputsMap[ValueFile] = new CInput(ValueFile, aGpio, ReadValue(ValueFile), mpIoService);

                if (0 == epoll_ctl(mEpoll, EPOLL_CTL_ADD, ValueFile, &pollEvent)) {
                    WatchInputs(false);
                    bRes = true;
                }
            }
        }
    }
    cout << "AddInput " << aGpio << " return " << bRes << endl;
    return bRes;
}

void CIO::WatchInputs (bool abSendSignal)
{
    // cout << "Watch..." << endl;
    epoll_event Event;
    while (0 < epoll_wait(mEpoll, &Event, 1, 0)) {
        ReadValue(Event.data.fd);   // We read value only to clear event. We assume value has changed
        CInput* pInput = InputsMap[Event.data.fd];
        if (!pInput->mbDebouncing)
        {
            pInput->mbDebouncing = true;
            pInput->mbValue = !pInput->mbValue;
            // cout << "Event on input " << pInput->mGpio << " value: " << pInput->mbValue << endl;
            if (abSendSignal) {
                InputSignal(pInput->mGpio, pInput->mbValue);

                pInput->mTimer.expires_from_now(*mpDebounce);
                pInput->mTimer.async_wait([=](const boost::system::error_code& error){
                    if (!error) {
                        OnDebounceEnd(pInput);
                    }
                });
            }
        }
        else {
            // cout << "Debouncing input " << pInput->mGpio << endl;
            pInput->mTimer.expires_from_now(*mpDebounce);
            pInput->mTimer.async_wait([=](const boost::system::error_code& error){
                if (!error) {
                    OnDebounceEnd(pInput);
                }
            });
        }
    }
}

bool CIO::ReadValue (int aFd)
{
    char Value;
    lseek(aFd, 0, SEEK_SET);
    read(aFd, &Value, 1);
    return ('1' == Value);
}

void CIO::OnDebounceEnd (CInput* pInput)
{
    pInput->mbDebouncing = false;
    bool Value = ReadValue(pInput->mFd);
    if (pInput->mbValue != Value) {
        pInput->mbValue = Value;
        InputSignal(pInput->mGpio, Value);
    }
}

void CIO::OnTimer (void)
{
    // cout << "OnTimer..." << endl;
    WatchInputs(true);
    
    // Reschedule the timer in the future:
    mpTimer->expires_from_now(*mpInterval);
    mpTimer->async_wait([this](const boost::system::error_code& error){
        if (!error) {
            OnTimer();
        }
    });
    // cout << "OnTimer END" << endl;
}

bool CIO::AddOutput (int aGpio, bool abValue)
{
    bool bRes = false;
    ofstream ExportFile("/sys/class/gpio/export");
    if (ExportFile.is_open()) {
        ExportFile << aGpio;
        ExportFile.close();

        // Must wait after export eiher /sys/class/gpio/gpioXX does not have right permission
        this_thread::sleep_for(chrono::milliseconds(200));

        string DirectionFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/direction";
        ofstream DirectionFile(DirectionFileName);
        if (DirectionFile.is_open()) {
            DirectionFile << "out";
            DirectionFile.close();
            OutputsTimers[aGpio] = TimerPtr (new boost::asio::deadline_timer(*mpIoService));
            bRes = SetOutput(aGpio, abValue);
        }
    }
    cout << "AddOutput " << aGpio << " return " << bRes << endl;
    return bRes;
}

bool CIO::SetOutput (int aGpio, bool abValue, int aDurationMs)
{
    bool bRes = false;
    string ValueFileName = string("/sys/class/gpio/gpio") + to_string(aGpio) + "/value";
    ofstream ValueFile(ValueFileName);
    if (ValueFile.is_open()) {
        ValueFile << abValue ? "1" : "0";
        ValueFile.close();
        bRes = true;
        if (aDurationMs) {
            boost::posix_time::millisec Interval(aDurationMs);
            OutputsTimers[aGpio]->expires_from_now(Interval);
            OutputsTimers[aGpio]->async_wait([this, aGpio, abValue](const boost::system::error_code& error) {
                if (!error) {
                    SetOutput(aGpio, !abValue);
                }
            });
        }
    }
    cout << "SetOutput " << aGpio << " to " << (abValue ? "1" : "0") << " return " << bRes << endl;
    return true;
}
