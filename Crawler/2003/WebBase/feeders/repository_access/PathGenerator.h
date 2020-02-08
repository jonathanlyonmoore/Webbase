/*
 * PathGenerator.h
 *
 * Given a canonical hostname, generates an appropriate path where the
 * respective  repository files should be stored.
 *
 * Author: Taher H. Haveliwala
 *
 */

#ifndef __PATH_GENERATOR_H__
#define __PATH_GENERATOR_H__

#include "TypeDetector.h"

class PathGenerator {

public:
  PathGenerator() { setPathPrefix(""); };
  PathGenerator(string prefix) { setPathPrefix(prefix); }

  /** path prefix attribute, e.g., the root of the repository tree */
  void setPathPrefix(string prefix) { m_prefix = prefix + '/'; }
  string getPathPrefix() { return m_prefix; }

  /** compute the relative path for the given canonical hostname and
      page type
  */
  string getPathFor(string canonicalHostname, TypeDetector::Type type);

private:
  string m_prefix;

};

#endif
