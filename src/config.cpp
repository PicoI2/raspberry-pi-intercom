#include "config.h"

#include <iostream>
#include <fstream>
#include <regex>

using namespace std;

CConfig Config;

void CConfig::SetParameter(string aParam, string aValue)
{
    mMap[aParam] = aValue;
}

bool CConfig::ReadConfigFile (string aFileName)
{
    bool bRes = false;
    ifstream ConfigFile (aFileName);
    if (ConfigFile.is_open()) {
        regex Regex ("^[ \t]*([^ \t\r\n\v\f=#]+)[ \t]*=[ \t]*([^ \t\r\n\v\f=#]+)");
        string Line;
        while(getline(ConfigFile, Line)) {
            smatch Match;
            if (regex_search(Line, Match, Regex)) {
                SetParameter(Match[1], Match[2]);
            }
        }
        bRes = true;
    }
    return bRes;
}

unsigned long CConfig::GetULong(string aParam, bool abMandatory)
{
    auto it = mMap.find(aParam);
    if (it != mMap.end()) {
        try {
            return stoul(it->second);
        }
        catch (const exception& e) {
            cerr << aParam << " '" << it->second << "' " << "not a number" << endl;    
            exit(0);
        }
    }
    else if (abMandatory) {
        cerr << "Missing " << aParam << " parameter" << endl;
        exit(0);
    }
    else {
        return 0;
    }
}

string CConfig::GetString(string aParam, bool abMandatory)
{
    auto it = mMap.find(aParam);
    if (it != mMap.end()) {
        return it->second;
    }
    else if (abMandatory) {
        cerr << "Missing " << aParam << " parameter" << endl;
        exit(0);
    }
    else {
        return "";
    }
}
