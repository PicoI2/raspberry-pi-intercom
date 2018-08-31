#pragma once

#include <iostream>

#include "httprequest.h"

using namespace std;

class CRequestHandler
{
public:
	CRequestHandler(CHttpRequest& aHttpRequest);
	string& Response();
	CHttpRequest& mHttpRequest;
	string mResponse;
};
