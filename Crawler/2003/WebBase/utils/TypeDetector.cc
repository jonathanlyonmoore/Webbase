/*
 * TypeDetector.cc
 *
 * Utility for detecting the type of a web page
 * Currently, only the extension of the url is used
 *
 * Author: Taher H. Haveliwala (taherh@cs.stanford.edu)
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>
#include <map>

#include "TypeDetector.h"
#include "url.h"

using namespace std;

string TypeDetector::TypeNames[TypeDetector::_TYPE_LEN] = 
  {"any", "text", "image", "audio", "video", "unknown"};
map<string, TypeDetector::Type> TypeDetector::NameTypes;

/** The below probably belongs in a conf file, but I plan to use 
    /etc/mime.types and /etc/mime-magic eventually 
    Btw the below looks ugly, but c++ sometimes be a pain
*/
void TypeDetector::setExtTypeMapping() {
  string t[] = {"htm", "html", "shtm", "shtml",
		"php", "php3", "phtml", "asp", 
		"txt", "jhtml", "stm",
		"no_ext"};
  set<string> text_ext(t, t+sizeof(t)/sizeof(t[0]));

  string a[] = {"mp3", "wav", "ra", "rm"};
  set<string> audio_ext(a, a+sizeof(a)/sizeof(a[0]));
  
  string i[] = {"jpg", "jpeg", "gif", "png", "bmp"};
  set<string> image_ext(i, i+sizeof(i)/sizeof(i[0]));

  loadExtSet(text_ext, TEXT);
  loadExtSet(audio_ext, AUDIO);
  loadExtSet(image_ext, IMAGE);
}

void TypeDetector::loadExtSet(set<string>& extensions,
			      Type type) {
  set<string>::const_iterator iter;
  for(iter = extensions.begin(); iter != extensions.end(); iter++) {
    // check to see that extension doesn't already have a mapping
    if(m_extTypeMap.find(*iter) != m_extTypeMap.end()) {
      error() << "Ambiguous type for extension " << *iter << endl;
      exit(1);
    }

    // check for special extensions
    if(*iter == "no_ext") {
      m_extTypeMap[""] = type;
    } else {
      m_extTypeMap[*iter] = type;
    }
  }
}

TypeDetector::Type TypeDetector::getTypeOf(const url* url, 
					   const char* buf, 
					   size_t buflen) const {
  map<string, Type>::const_iterator elt = 
    m_extTypeMap.find(makeLower(extractExtension(url)));
  if(elt == m_extTypeMap.end()) return UNKNOWN;
  else return elt->second;
}

string TypeDetector::extractExtension(const url* url) {
  const char* path        = url->path;
  int len = strlen(path);
  const char* ext_end     = path + len - 1;
  const char* ext_start;
  char c = '*';

  for (ext_start = ext_end; path < ext_start; --ext_start) {
    c = *ext_start;
    if (c == '.' || c == '/') break;
  }
  if (c == '.') {
    int ext_length = ext_end - ++ext_start;
    if (ext_length > 5 || ext_length <= 0) return "";
    return ext_start;
  }
  return "";
}

string TypeDetector::makeLower(string s) {
  string::iterator iter;
  string r;
  for(iter = s.begin(); iter != s.end(); iter++) {
    r += tolower(*iter);
  }
  return r;
}
