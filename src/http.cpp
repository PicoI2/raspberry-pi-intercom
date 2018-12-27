#include "http.h"
#include <iostream>
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
    {"ico", ICO},
    {"jpeg", JPEG},
    {"jpg", JPEG},
    {"js", JS},
    {"mp3", MP3},
    {"pbg", PNG},
    {"ttf", TTF},
    {"svg", SVG},
    {"wav", WAV},
    {"woff", WOFF},
    {"woff2", WOFF2},
};

string GetMimeType(string aUri)
{
    size_t PosLastDot = aUri.find_last_of('.');
    if (string::npos != PosLastDot) {
        string Extension = aUri.substr(PosLastDot);
        if (Extension.size()>1) {
            Extension = Extension.substr(1);    // Remove the '.'
        }
        boost::algorithm::to_lower(Extension);

        for (ExtensionToMime Ext : ExtensionsArray) {
            if (Extension == Ext.Extension) {
                return Ext.Mime;
            }
        }
        cerr << "Unknown mime type :'" << aUri << "' file";
    }

    return "";
}

}
}