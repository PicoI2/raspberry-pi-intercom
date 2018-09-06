#pragma once
#include <map>
#include <string>

using namespace std;

class CConfig {
public :
    void SetParameter(string aParam, string aValue);
    bool ReadConfigFile (string aFileName);
    map<string, string> Map;
};

extern CConfig Config;
