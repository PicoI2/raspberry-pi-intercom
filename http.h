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
	namespace mime {
		const static string JS   = "application/javascript\r\n";
		const static string CSS  = "text/css\r\n";
		const static string HTML = "text/html\r\n";

		string GetMimeType(string aExtension);
	}
}
