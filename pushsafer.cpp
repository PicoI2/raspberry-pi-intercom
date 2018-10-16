#include "pushsafer.h"
#include <iostream>

#include <chrono>
#include <thread>

using namespace std;

CPushSafer PushSafer;

bool CPushSafer::Init (boost::asio::io_service* apIoService)
{
    mpIoService = apIoService;
    return true;
}

bool CPushSafer::Notification ()
{
//     string json = 
// R"V0G0N({
//     "k": "pushsaferprivatekey",
//     "d": 1337,
//     "t": "CPushsafer",
//     "m": "This is a test from sending in C++ with boost"
// })V0G0N";

    string formdata = 
"--------------------------3173acd4f807267a\r\n\
Content-Disposition: form-data; name=\"m\"\r\n\
\r\n\
Hello World!\r\n\
--------------------------3173acd4f807267a\r\n\
Content-Disposition: form-data; name=\"k\"\r\n\
\r\n\
pushsaferprivatekey\r\n\
--------------------------3173acd4f807267a\r\n\
Content-Disposition: form-data; name=\"d\"\r\n\
\r\n\
1337\r\n\
--------------------------3173acd4f807267a--\r\n\
";

    string request =
"POST /api HTTP/1.1\r\n\
Host: www.pushsafer.com\r\n\
User-Agent: C/1.0\r\n\
Accept: */*\r\n\
Content-Length: ";
    request.append(to_string(formdata.length()));
    request.append(
"\r\n\
Expect: 100-continue\r\n\
Content-Type: multipart/form-data; boundary=------------------------3173acd4f807267a\r\n\
\r\n");
    request.append(formdata);

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


    tcp::resolver Resolver(*mpIoService);
    tcp::resolver::query Query("www.pushsafer.com", "http");
    tcp::resolver::iterator EndpointIterator = Resolver.resolve(Query);
    tcp::resolver::iterator End;

    tcp::socket Socket(*mpIoService);
    boost::system::error_code error = boost::asio::error::host_not_found;
    Socket.connect(*EndpointIterator, error);

    if (error) {
        tcp::endpoint endpoint = *EndpointIterator;
        cout << "Error connecting to: " << endpoint << endl;
    }
    else {
        boost::asio::write(Socket, boost::asio::buffer(request));
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        std::array<char, 4096> Buffer;
        size_t Read = Socket.read_some(boost::asio::buffer(Buffer));
        std::string Reply(Buffer.begin(), Buffer.begin() + Read);
        cout << Reply << endl;;
    }
    Socket.close();
}