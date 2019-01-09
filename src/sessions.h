#pragma once
#include <set>
#include <string>

using namespace std;

class CSessions {
public :
    string CreateSession ();
    bool IsSessionExist (string aSessionId);
    void DeleteSession (string aSessionId);
protected :
    set<string> mSessions;
};

extern CSessions Sessions;
