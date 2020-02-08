/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 1999-2003 The Board of Trustees of the
   Leland Stanford Junior University
   
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
/** Utilities for handling HTTP replies. */
/**
   HTTPUtil.cc - Utility functions for handling HTTP server replies.
   Wang Lam <wlam@cs.stanford.edu>
   January 2003
*/

#include <string>
#include <cstring>
#include <cstddef>
#include <cassert>

#include "HTTPUtil.h"

std::string getHTTPHeader(const char* httpBuff, size_t bufSize)
{
   const char *ignored;
   return getHTTPHeader(httpBuff, bufSize, &ignored);
}

std::string getHTTPHeader(const char* httpBuff, size_t bufSize,
                          const char** data)
{
   const char header_start[] = "HTTP/";
   const size_t header_start_len = sizeof(header_start) - 1;
   if ((bufSize < header_start_len) || 
       (strncmp(httpBuff,header_start,header_start_len) != 0)) {
      // HTTP/0.9
      *data = httpBuff;
      return std::string("");
   } else {
      // RFC 2616 sec 19.3 recommends not requiring the CR in CR LF.
      const char CR = 0x0D;
      const char LF = 0x0A;
      int newlines = 0;
   
      // start searching at pageBuffer+1, to make the loop convenient
      const char *ptr = httpBuff;
      size_t hdr_len = 1;
      while (++hdr_len <= bufSize) {
         ++ptr;
         switch (*ptr) {
            default: newlines = 0;
            case CR: continue;

            case LF: ++newlines;
                     if (newlines != 2) continue;
         }
         break;
      };

      // Body starts right after two newlines; this may point
      // past end of buffer if newlines occur at end of buffer,
      // or don't occur at all.  If body-httpBuff>=bufSize, no body.
      if (newlines != 2) --hdr_len;  // unroll last while increment
      *data = ptr+1;
      return std::string(httpBuff,hdr_len);
   }
}

/* Example use
#include <iostream>

int main(void)
{
    // char hi[] = "Hello, there";
    // char hi[] = "HTTP/\r\n\r\nBody";
    char hi[] = "HTTP/1.0 200 Happy\n\nt";
    const char *foo;
    std::string hdr(getHTTPHeader(hi,sizeof(hi),&foo));

    std::cout << "Header >" << hdr << "<" << std::endl;
    std::cout << "Body >" << std::string(foo,sizeof(hi)-(foo-hi)) << "<" 
	    << std::endl;
    return 0;
}
*/

