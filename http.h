#pragma once

#include <string>
using namespace std;

namespace http {
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
	namespace code {
		const static string OK = "HTTP/1.1 200 OK\r\n";
		const static string BAD_REQUEST = "HTTP/1.1 400 Bad Request\r\n";
		const static string NOT_FOUND = "HTTP/1.1 404 Not Found\r\n";
	}
	namespace mime {
		const static string JS   = "application/javascript\r\n";
		const static string CSS  = "text/css\r\n";
		const static string HTML = "text/html\r\n";

		string GetMimeType(string aExtension);
	}
}
