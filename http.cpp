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

string GetMimeType(string aExtension)
{
	string Extension = aExtension;
	boost::algorithm::to_lower(Extension);

	ExtensionToMime Ext;
	for (size_t i=0; i< sizeof (ExtensionsArray) / sizeof(http::mime::ExtensionsArray[0]); ++i) {
		Ext = ExtensionsArray[i];
		if (Extension == Ext.Extension) {
			return Ext.Mime;
		}
	}

	return "";
}

}
}