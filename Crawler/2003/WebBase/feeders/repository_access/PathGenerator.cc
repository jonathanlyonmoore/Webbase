/*
 * PathGenerator.cc
 *
 * Given a canonical hostname, generates an appropriate path where the
 * respective  repository files should be stored.
 *
 * Author: Taher H. Haveliwala
 *
 */

#include "PathGenerator.h"

#include <cassert>

string PathGenerator::getPathFor(string canonicalHostname, 
				 TypeDetector::Type type) {
  cerr << "getPathFor: " << canonicalHostname << endl;
  string revpath;
  int last = canonicalHostname.size() - 1;
  int begin = 0;
  while( (begin = canonicalHostname.rfind('.', last)) != string::npos) {
    string component = canonicalHostname.substr(begin+1, last - begin);
    if(component == "") { component = "_dot_"; }
    revpath += component + '/';
    last = begin-1;
    if(last < 0) break;
  }
  string component = canonicalHostname.substr(0, last+1);
  if(component == "") { component = "_dot_"; }
  revpath += component + '/';
  
  string r = m_prefix + revpath + "_repository_/" + TypeDetector::TypeNames[type];
  cerr << "path: " << r << endl;
  return r;
}
