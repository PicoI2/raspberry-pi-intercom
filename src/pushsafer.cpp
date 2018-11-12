#include "pushsafer.h"
#include <iostream>
#include <chrono>
#include <thread>
#include "config.h"

using namespace std;

CPushSafer PushSafer;

bool CPushSafer::Init (boost::asio::io_service* apIoService)
{
    mpIoService = apIoService;
    return true;
}

string CPushSafer::ToFormData (string aKey, string aValue)
{
    string Param = "--------------------------3173acd4f807267a\r\nContent-Disposition: form-data; name=\"";
    Param.append(aKey);
    Param.append("\"\r\n\r\n");
    Param.append(aValue);
    Param.append("\r\n");
    return Param;
}

bool CPushSafer::Notification (string aMessage)
{
// k = Private or Alias Key*
// d = Device
// t = Title
// m = Message*
// s = Sound empty=device default or a number 0-50
// v = Vibration empty=device default or a number 1-3
// i = Icon Standard = 1 or a number 1-177
// c = Icon Color Standard = Empty
// u = URL/Link u="https://www.pushsafer.com"
// ut = URL Title ut="Open Link"
// p = Picture Data URL with Base64-encoded string
// p2 = Picture 2 Data URL with Base64-encoded string
// p3 = Picture 3 Data URL with Base64-encoded string
// data:image/gif;base64,R0l...BOw==
// data:image/jpeg;base64,C4s...Cc1==
// data:image/png;base64,G0G...h5r==
// l = Time to Live Integer number 0-43200: Time in minutes,
// after which message automatically gets purged.
// pr = Priority
// -2 = lowest priority
// -1 = lower priority
// 0 = normal priority
// 1 = high priority
// 2 = highest priority
// re = Retry / resend
// Integer 60-10800 (60s steps): Time in seconds, after a message shuld resend.
// ex = Expire
// Integer 60-10800: Time in seconds, after the retry/resend should stop.
// a = Answer
// 1 = Answer is possible
// 0 = Answer is not possible.
// * required parameters
// max. size of all POST parameters = 8192kb

    string FormData = ToFormData("k", Config.GetString("pushsafer-key"));
    FormData.append(ToFormData("t", "rpi-intercom"));
    FormData.append(ToFormData("m", aMessage));
    string Device = Config.GetString("pushsafer-device", false);
    if (!Device.empty()) {
        FormData.append(ToFormData("d", Device));
    }
    FormData.append(ToFormData("s", "17"));
    FormData.append(ToFormData("v", "3"));
    FormData.append(ToFormData("pr", "2"));
    FormData.append("--------------------------3173acd4f807267a--\r\n");

    string Request =
"POST /api HTTP/1.1\r\n\
Host: www.pushsafer.com\r\n\
User-Agent: C/1.0\r\n\
Accept: */*\r\n\
Content-Length: ";
    Request.append(to_string(FormData.length()));
    Request.append(
"\r\n\
Expect: 100-continue\r\n\
Content-Type: multipart/form-data; boundary=------------------------3173acd4f807267a\r\n\
\r\n");
    Request.append(FormData);

    tcp::resolver Resolver(*mpIoService);
    tcp::resolver::query Query("www.pushsafer.com", "http");
    try {
        tcp::resolver::iterator EndpointIterator = Resolver.resolve(Query);

        tcp::socket Socket(*mpIoService);
        boost::system::error_code error = boost::asio::error::host_not_found;
        Socket.connect(*EndpointIterator, error);

        if (error) {
            tcp::endpoint endpoint = *EndpointIterator;
            cout << "Error connecting to: " << endpoint << endl;
        }
        else {
            boost::asio::write(Socket, boost::asio::buffer(Request));
            
            // TODO Do something else
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));

            std::array<char, 4096> Buffer;
            size_t Read = Socket.read_some(boost::asio::buffer(Buffer));
            std::string Reply(Buffer.begin(), Buffer.begin() + Read);
            cout << Reply << endl;;
        }
        Socket.close();
    }
    catch (std::exception& e) {
        cerr << "Cannot access " << Query.host_name() << endl;
    }
}