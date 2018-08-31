#include "http.h"

#include <boost/algorithm/string.hpp>

namespace http {
namespace mime {

struct ExtensionToMime
{
    string Extension;
    string Mime;
};
ExtensionToMime ExtensionsArray[] = {
    {"css", CSS},
    {"html", HTML},
    {"js", JS},
};

string GetMimeType(string aUri)
{
    string Extension = aUri.substr(aUri.find_last_of('.'));
    if (Extension.size()>1) {
        Extension = Extension.substr(1);	// Remove the '.'
    }
    boost::algorithm::to_lower(Extension);

    for (ExtensionToMime Ext : ExtensionsArray) {
        if (Extension == Ext.Extension) {
            return Ext.Mime;
        }
    }

    return "";
}

}
}