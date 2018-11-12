#pragma once
#include <map>
#include <string>

using namespace std;

class CConfig {
public :
    void SetParameter(string aParam, string aValue);
    bool ReadConfigFile (string aFileName);
    unsigned long GetULong(string aParam, bool abMandatory=true);
    string GetString(string aParam, bool abMandatory=true);
protected :
    map<string, string> mMap;
};

extern CConfig Config;
