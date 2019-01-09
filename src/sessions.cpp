#include "sessions.h"

#include <iostream>
#include <uuid/uuid.h>

using namespace std;

CSessions Sessions;

string CSessions::CreateSession ()
{
    uuid_t uuid;
    char out [64];
    uuid_generate(uuid);
    uuid_unparse(uuid, out);
    string SessionId = out;

    if (!IsSessionExist(SessionId)) {
        mSessions.insert(SessionId);
    }
    else {
        SessionId = CreateSession();
    }
    return SessionId;
}

bool CSessions::IsSessionExist (string aSessionId)
{
    return (mSessions.end() != mSessions.find(aSessionId));
}

void CSessions::DeleteSession (string aSessionId)
{
    mSessions.erase(aSessionId);
}
