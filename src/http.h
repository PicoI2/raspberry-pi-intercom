#pragma once

#include <string>
using namespace std;

namespace http {

    string GetCookie (string aCookies, string aName);
    
    namespace method {
        const static string GET = "GET";
        const static string HEAD = "HEAD";
        const static string POST = "POST";
        const static string PUT = "PUT";
        const static string DELETE = "DELETE";
        const static string CONNECT = "CONNECT";
        const static string OPTIONS = "OPTIONS";
        const static string TRACE = "TRACE";
    }
    namespace mime {
        const static string JS      = "application/javascript";
        const static string MP3     = "audio/mpeg";
        const static string WAV     = "audio/wav";
        const static string TTF     = "font/ttf";
        const static string WOFF    = "font/woff";
        const static string WOFF2   = "font/woff2";
        const static string JPEG    = "image/jpeg";
        const static string PNG     = "image/png";
        const static string SVG     = "image/svg+xml";
        const static string ICO     = "image/x-icon";
        const static string CSS     = "text/css";
        const static string HTML    = "text/html";

        string GetMimeType(string aExtension);
    }
    
}
