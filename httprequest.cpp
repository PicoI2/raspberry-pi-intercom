#include <iostream>
#include <regex>
#include "httprequest.h"
#include "http.h"

using namespace std;

// Constructor
CHttpRequest::CHttpRequest(string& aRequest)
    : mRequest(aRequest)
{
    Parse();
}

// Constructor
void CHttpRequest::Parse ()
{
    bool bRes = false;

    istringstream RequestLines (mRequest);
    string RequestLine;
    if (getline(RequestLines, RequestLine)) {
        regex Regex("(GET|HEAD|POST|PUT|DELETE|CONNECT|OPTIONS|TRACE) ([^ ]+) (HTTP[^ ]+)");
        smatch Matches;
        if (regex_search(RequestLine, Matches, Regex)) {
            mMethod = Matches[1];
            mUri    = Matches[2];
            bRes = true;
        }
    }
    if (bRes) {
        while (getline(RequestLines, RequestLine)) {
            regex Regex("([^ :]+): *(.+)");
            smatch Matches;
            if (regex_search(RequestLine, Matches, Regex)) {
                mHeaderMap[Matches[1]] = Matches[2];
            }
        }

        mMimeType = http::mime::GetMimeType(mUri);
    }

    cout << mMethod << " " << mUri  << " mime:'" << mMimeType << "'" << endl;
}
