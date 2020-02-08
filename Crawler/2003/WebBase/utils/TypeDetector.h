/*
 * TypeDetector.h
 *
 * Utility for detecting the type of a web page
 * Currently, only the extension of the url is used
 *
 * Author: Taher H. Haveliwala (taherh@cs.stanford.edu)
 *
 */

#ifndef __TYPE_DETECTOR_H__
#define __TYPE_DETECTOR_H__

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
#include <string>
#include <map>
#include <set>

#include "url.h"

/*
 * getTypeOf provides the type detection functionality
 */
class TypeDetector {
  
public:
  TypeDetector() { setExtTypeMapping(); }

  /** This enum should eventually reflect standard MIME types.  Remember to 
      keep TypeNames in sync (unfortunately c++ doesn't allow inline 
      initialization.
  */
  enum Type {ANY, TEXT, IMAGE, AUDIO, VIDEO, UNKNOWN, _TYPE_LEN};
  static std::string TypeNames[_TYPE_LEN];
  static std::map<std::string, Type> NameTypes;

  static void init() {
    int i;
    for(i = ANY; i < _TYPE_LEN; i++) {
      NameTypes[TypeNames[i]] = Type(i);
    }
  }

  /** returns the current extension-based type mapping */
  std::map<std::string, Type> getExtMapping() const { return m_extTypeMap; }

  /** return the type detected for the url and buffer */
  Type getTypeOf(const url* url, const char* buf, size_t buflen) const;
  Type getTypeOf(const url* url) const {
    return getTypeOf(url, NULL, 0);
  }
  Type getTypeOf(std::string url, const char* buf, size_t buflen) const {
    static_url u;
    u.replace(url.c_str(), url.c_str() + url.size());
    return getTypeOf(&u, buf, buflen);
  }
  Type getTypeOf(std::string url) const {
    return getTypeOf(url, NULL, 0);
  }

  /** A collection of predicates on types.  Based on these predicates, other
      modules may choose various policies on how to deal with various
      data objects.
  */
  bool isBinary(Type t);
  bool isText(Type t);

  /* retrieve the extension of a url */
  static std::string extractExtension(const url* url);
  static std::string extractExtension(std::string url) {
    static_url u;
    u.replace(url.c_str(), url.c_str() + url.size());
    return extractExtension(&u);
  }

  static std::string makeLower(std::string s);
  
private:
  /** loads the default extension-based type mapping */
  void setExtTypeMapping();

  /** specify an additional set of extensions of a type */
  void loadExtSet(std::set<std::string>& extensions, Type type);

  std::map<std::string, Type> m_extTypeMap;

  std::ostream& error() {
    return (std::cerr << "Error in TypeDetector: ");
  }
};

#endif
