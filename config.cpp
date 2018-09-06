#include "config.h"

#include <iostream>
#include <fstream>
#include <regex>

using namespace std;

CConfig Config;

void CConfig::SetParameter(string aParam, string aValue)
{
    Map[aParam] = aValue;
}

bool CConfig::ReadConfigFile (std::string aFileName)
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

