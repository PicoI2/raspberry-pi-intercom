#include <iostream>
#include <fstream>
#include <sstream>
#include "requesthandler.h"
#include "http.h"
#include "io.h"

// Constructor
CRequestHandler::CRequestHandler(CHttpRequest& aHttpRequest)
:mHttpRequest(aHttpRequest)
{
}

string& CRequestHandler::Response()
{
    mResponse = http::code::NOT_FOUND;
    if (mHttpRequest.mMethod == http::method::GET && !mHttpRequest.mMimeType.empty() && string::npos == mHttpRequest.mUri.find("..")) {
        ifstream RequestedFile("./www/" + mHttpRequest.mUri, std::ios::in | std::ios::binary);
        if (RequestedFile.is_open()) {
            mResponse = http::code::OK;
            mResponse.append("Content-Type: " + mHttpRequest.mMimeType + "\r\n\r\n");
            char buffer [1024];
            while (RequestedFile.read(buffer, sizeof(buffer)).gcount() > 0) {
                mResponse.append(buffer, RequestedFile.gcount());
            }
        }
    }
    else if (mHttpRequest.mMethod == http::method::PUT) {
        if ("/dooropen" == mHttpRequest.mUri) {
            if (IO.SetOutput(26, true, 2000))
            {
                mResponse = http::code::OK;
            }
        }
    }
    // cout << "Response size: " << mResponse.size() << endl;
    return mResponse;

}
