#pragma once

#include <iostream>
#include <map>

using namespace std;

class CHttpRequest
{
public:
	CHttpRequest(string& aRequest);
	void Parse ();
	string& mRequest;
	string mMethod;
	string mUri;
	string mMimeType;
	map<string, string> mHeaderMap;
};
