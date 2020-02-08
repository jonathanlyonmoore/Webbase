/* 
 * normalize.c - Wang Lam <wlam@cs.stanford.edu> Feb 2001
 *
 * The normalize function takes URLs and converts them into a "canonical" form
 * so that ( normalize(A), normalize(B), strcmp(A,B)==0 ) if and only if
 * URLs A and B were references to the same resource.
 *
 * This normalize function uses W3C's libwww to normalize, replacing our
 * prior hand-normalization code.  The principle is similar: remove
 * redundant port numbers, . and .. paths, and so on, but now it's up to
 * W3C spec.  :)
 *
 * This file supersedes the normalize.cc it postdates, and conforms to the
 * C declaration of normalize.h.  The comments in this file supersede the
 * comments in normalize.h.
 *
 */

#include "normalize.h"
#include <string.h>

#define HAVE_CONFIG_H
#include <WWWCore.h>
#include <HTParse.h>


/* int normalize(char *url);
   Normalize NUL-terminated url in-place if possible, returning 1.
   Mimicking prior-version normalize() behavior, returns 0 (doing nothing
   to url) if url is not an HTTP URL.
   This function is whitespace-sensitive.  (" http://www.yahoo.com/" is
   considered an invalid URL.)
 */
int normalize(char *url)
{
   if (url == NULL) return 0;
   if (strncasecmp(url,"http://",7) != 0)
      return 0;
   HTSimplify(&url);
   return 1;
}

