/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 1999-2002 The Board of Trustees of the
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
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "url.h"

const int MAX_URL = 10*1024;


#define STRCPY(d, s, sp)  { register char c;                                  \
                            while ((c = *s++) != sp) *d++ = c; \
                            *d++ = 0; }

static inline char* copy_encoded(char* d, const char* de, 
		 	         const char* s, const char* se)
{
   register unsigned char c;

   while (s < se && d < de) {
      c = *s++;
      if (c > 0x20 && c != 0x7f) {
	 *d++ = c;
      } else if (c == 0) {
         break;
      } else { // c <= 0x20 || c == 0x7f;
	 if (d + 3 < de) {
	    *d++ = '%';
	    *d++ = (c / 0x10 + '0');
	    *d++ = (c % 0x10 + '0');
	 } else {
	    return NULL;
	 }
      }
   }
   if (d < de) {
      *d++ = 0;
      return d;
   }

   return NULL;
}

static const char* empty_string = "";
static const char* slash_string = "/";


/* 
 * Base Url definitions
 */ 
 
url::url()
   : scheme(empty_string), net_loc(empty_string),
     port(empty_string), path(empty_string), param(empty_string),
     query(empty_string), fragment(empty_string)
{}


url::url(const char* a, const char* b, const char* c, 
	 const char* d, const char* e, const char* f, 
	 const char* g)
{
   scheme   = a;
   net_loc  = b;
   port     = c;
   path     = d;
   param    = e;
   query    = f;
   fragment = g;
}

void url::initialize()
{
   scheme = empty_string;
   net_loc = empty_string;
   port = empty_string;
   path = empty_string;
   param = empty_string;
   query = empty_string;
   fragment = empty_string;
}

char* url::parse(const char* url_start, const char* url_end, 
		 char* buffer, int buffer_size)
{
   register char c, *d;
   register const char *s;
   char *buffer_end = buffer + buffer_size;

   int question = 0, semi = 0, colon = 0;
   
   // remove leading and trailing spaces
   while (url_start < url_end && isspace(c = *url_start))   url_start++;
   while (url_start < url_end && isspace(c = *(url_end-1))) url_end--;
   
   // check whether url is too long
   if (url_end - url_start >= buffer_size - 10) {
      return NULL;
   }

   // initialize url components
   fragment = port = query = param = path = net_loc = scheme = empty_string;
   if (url_start >= url_end)  return buffer;

   d = buffer;

   // identify fragment identifier and
   // check whether reserved characters exist
   s = url_start; c = 0;
   while (s < url_end && (c = *s) != '#') {
      if (c == '?') question = 1;
      if (c == ';') semi = 1;
      if (c == ':') colon = 1;
      s++;
   }
   if (c == '#') {
      const char* temp = s++;
      fragment = d;
      if ((d = copy_encoded(d, buffer_end, s, url_end)) == NULL)  return NULL;
      url_end = temp;
   }

   // identify scheme
   if (colon) {
      s = url_start; c = 0;
      while (s < url_end && (c = *s) != ':' &&
	     (('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') ||
	      ('0' <= c && c <= '9') || c == '+' || c == '-' || c == '.'))
	 s++;
      if (c == ':') {
	 scheme = d;
	 if ((d = copy_encoded(d, buffer_end, url_start, s)) == NULL)  return NULL;
         url_start = ++s;
      }
   }

   // identify network location
   if (url_start <= url_end - 2 && 
       *url_start == '/' && *(url_start+1) == '/') {
      s = url_start+2;
      net_loc = d;
      /* copy uncapitalized net_loc */
      while (s < url_end && (c = *s) != '/') {
	 if ('A' <= c && c <= 'Z') { c += ('a' - 'A'); }
	 if (c == ':') { c = 0; port = d + 1; }
	 *d++ = c;
	 s++;
      }
      // remove the last dot
      if (net_loc < d && *(d-1) == '.') d--;
      *d++ = 0;
      url_start = s;
   }

   // identify query 
   if (question) {
      s = url_start;
      while (s < url_end && (c = *s) != '?') s++;
      if (c == '?') {
	 const char* temp = s++;
	 query = d;
	 if ((d = copy_encoded(d, buffer_end, s, url_end)) == NULL)  return NULL;
	 url_end = temp;
      }
   }

   // identify parameter
   if (semi) {
      s = url_start;
      while (s < url_end && (c = *s) != ';') s++;
      if (c == ';') {
	 const char* temp = s++;
	 param = d;
	 if ((d = copy_encoded(d, buffer_end, s, url_end)) == NULL)  return NULL;
	 url_end = temp;
      }
   }

   // copy path
   s = url_start;
   if (s == url_end) {
      path = slash_string;
   } else {
      path = d;
      if ((d = copy_encoded(d, buffer_end, s, url_end)) == NULL) return NULL;
   }

   return d;
}


char* url::merge(const url& base, char* buffer, int buffer_size)
{
   register const char* s;
   register       char* d = buffer;

   // check if any of the URLs are empty
   if (base.scheme[0] == 0 && base.net_loc[0] == 0 && base.path[0] == 0 &&
       base.param[0] == 0 && base.query[0] == 0) { return d; }
   if (scheme[0] == 0 && net_loc[0] == 0 && path[0] == 0 &&
       param[0] == 0 && query[0] == 0) {
      scheme = base.scheme;
      net_loc = base.net_loc;
      port = base.port;
      path = base.path;
      param = base.param;
      query = base.query;
      fragment = base.fragment;
      return d; 
   }

   // scheme
   if (scheme[0] != 0) { 
      return d;
   } else {
      scheme = base.scheme;
   }
   
   // net location
   if (net_loc[0] != 0) {
      return d;
   } else {
      net_loc = base.net_loc;
      if (base.port[0] != 0) {
	 port = base.port;
      }
   }

   // path
   if (path[0] == '/') {
      return d;
   }
   if (path[0] == 0) {
      path = base.path;
      
      // parameter
      if (param[0] != 0) {
	 return d;
      }
      if (base.param[0] != 0) {
	 param = base.param;
      }

      // query
      if (query[0] != 0) {
	 return d;
      }
      if (base.query[0] != 0) {
	 query = base.query;
      }
   }

   // merge two paths 
   {
      const int START = 0;
      //const int ONE_DOT = START+1;
      const int TWO_DOT = START+2;

      int  state;
      int  depth;
      register char c;

      state = START, depth = 0;
      for (s = path; (c = *s) != 0; s++) {
	 switch (c) {
	    case '.':
	       if (state != TWO_DOT) {
		  ++state;
	       } else {
		  s -= 2;
		  goto start_merge;
	       }
	       break;
	    case '/':
	       if (state == TWO_DOT) {
		  depth++;
	       }
	       state = START;
	       break;
	    default:
	       if (state != START) {
		  s -= state;
	       }
	       goto start_merge;
	 }
      }
   start_merge:
      {
	 const char *rel_start = s, *base_end;
	 for (s = base.path + strlen(base.path) - 1; 
	      depth >= 0 && s >= base.path; s--) {
	    if (*s == '/') depth--;
	 }
	 base_end = s + 2;
	 if (base_end - base.path + depth*3 + (int)strlen(rel_start) + 1 
	     > buffer_size) {
	    return NULL;
	 }

	 // set path
	 path = d;

	 // add initial /.. 
	 if (depth >= 0) {
	    *d++ = '/';
	    *d++ = '.';
	    *d++ = '.';
	    depth--;
	 }

	 // copy base path
	 //*d++ = '|'; // DEBUG
	 for(s = base.path; s < base_end;)  *d++ = *s++;

	 // copy relative path
	 //*d++ = '|'; // DEBUG
	 s = rel_start;
	 STRCPY(d, s, 0);
      }
   }
   return d;
}


char* url::store(char* buffer, int buffer_size)
{
   register const char *s;
   register char       *d;

   int new_buffer_size = 
      strlen(scheme) + 1 + strlen(net_loc) + 1 + strlen(port) + 1 + 
      strlen(path) + 1 + strlen(param) + 1 + strlen(query) + 1 + 
      strlen(fragment) + 1;

   if (buffer_size < new_buffer_size) {
      return NULL;
   }

   d = buffer;

   s = scheme;
   scheme = d;
   STRCPY(d, s, 0);

   s = net_loc;
   net_loc = d;
   STRCPY(d, s, 0);

   s = port;
   port = d;
   STRCPY(d, s, 0);

   s = path;
   path = d;
   STRCPY(d, s, 0);

   s = param;
   param = d;
   STRCPY(d, s, 0);

   s = query;
   query = d;
   STRCPY(d, s, 0);

   s = fragment;
   fragment = d;
   STRCPY(d, s, 0);

   return d;
}

int url::strcpy(char* buffer, const char* be) const
{
   register const char *src;
   register char       *b = buffer;
   char  c;

   src = scheme;
   if (*src != 0) {
      while ((c = *src++) != 0 && b < be) *b++ = c;
      if (b < be) *b++ = ':';
   }

   src = net_loc;
   if (*src != 0) {
      if (b < be) *b++ = '/';
      if (b < be) *b++ = '/';
      while ((c = *src++) != 0 && b < be) *b++ = c;
   }

   src = port;
   if (*src != 0 && strcmp(src, "80") != 0) {
      if (b < be) *b++ = ':';
      while ((c = *src++) != 0 && b < be) *b++ = c;
   }

   src = path;
   while ((c = *src++) != 0 && b < be) *b++ = c;

   src = param;
   if (*src != 0) {
      if (b < be) *b++ = ';';
      while ((c = *src++) != 0 && b < be) *b++ = c;
   }

   src = query;
   if (*src != 0) {
      if (b < be) *b++ = '?';
      while ((c = *src++) != 0 && b < be) *b++ = c;
   }

   if (b < be) {
      *b = 0;
   } else {
      *--b = 0;
   }

   return b - buffer;
}


/*
 * dynamic url definitions
 */

dynamic_url::dynamic_url()
   : url(), buffer(NULL), buffer_size(0)
{}

dynamic_url::dynamic_url(const char* a, const char* b, const char* c, 
			 const char* d, const char* e, const char* f, 
			 const char* g)
   : url(a, b, c, d, e, f, g), buffer(NULL), buffer_size(0)
{
   if (allocate_space() == NULL) {
      initialize();
      return;
   }
   if (store(buffer, buffer_size) == NULL) {
      initialize();
   }
}

dynamic_url::dynamic_url(const url& u)
   : url(), buffer(NULL), buffer_size(0)
{
   *this = u;
}

dynamic_url::dynamic_url(const dynamic_url& u)
   : url(), buffer(NULL), buffer_size(0)
{
   *this = u;
}

void dynamic_url::operator = (const dynamic_url& u)
{
   *this = (const url&)u;
}

void dynamic_url::operator = (const url& u)
{
   scheme   = u.scheme;
   net_loc  = u.net_loc;
   port     = u.port;
   path     = u.path;
   param    = u.param;
   query    = u.query;
   fragment = u.fragment;

   if (allocate_space() == NULL) {
      initialize();
      return;
   }
   if (store(buffer, buffer_size) == NULL) {
      initialize();
      return;
   }
}

dynamic_url::~dynamic_url()
{
   if (buffer != NULL) {
      free(buffer);
      buffer = NULL;
   }
}

int dynamic_url::replace(const char* scheme, const char* net_loc, 
			const char* port,   const char* path, 
			const char* param,  const char* query, 
			const char* fragment)
{
   this->scheme = scheme;
   this->net_loc = net_loc;
   this->port = port;
   this->path = path;
   this->param = param;
   this->query = query;
   this->fragment = fragment;

   if (allocate_space() == NULL) {
      initialize();
      return -1;
   }
   if (store(buffer, buffer_size) == NULL) {
      initialize();
      return -1;
   }
   
   return 0;
}

int dynamic_url::replace(const char* start, const char* end)
{
   char tmp_buffer[MAX_URL];
   
   // check if the url is too long
   if (end - start > MAX_URL - 10)  return -1;

   // parse the url
   if (parse(start, end, tmp_buffer, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   // allocate buffer space
   if (allocate_space() == NULL) {
      initialize();
      return -1;
   }
   if (store(buffer, buffer_size) == NULL) {
      initialize();
      return -1;
   }
   
   return 0;
}

int dynamic_url::replace(const char* start, const char* end, const url& base)
{
   char tmp_buffer[MAX_URL];
   char path[MAX_URL];

   // check if the url is too long
   if (end - start > MAX_URL - 10)  return -1;

   if (parse(start, end, tmp_buffer, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   if (merge(base, path, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   if (allocate_space() == NULL) {
      initialize();
      return -1;
   }
   if (store(buffer, buffer_size) == NULL) {
      initialize();
      return -1;
   }
   return 0;
}
   

char* dynamic_url::allocate_space()
{
   int size = 
      strlen(scheme) + 1 + strlen(net_loc) + 1 + strlen(port) + 1 + 
      strlen(path) + 1 + strlen(param) + 1 + strlen(query) + 1 + 
      strlen(fragment) + 1;

   if (buffer == NULL || buffer_size < size) {
      if (buffer != NULL) {
	 free(buffer);
      }
      buffer = (char*)malloc(size);
      if (buffer == NULL) {
	 buffer_size = 0;
	 return NULL;
      }
      buffer_size = size;
   }

   return buffer;
}
   

/*
 * Static Url definitions
 */

static_url::static_url()
   : url()
{}

static_url::static_url(const char* a, const char* b, const char* c, 
		       const char* d, const char* e, const char* f, 
		       const char* g)
   : url(a, b, c, d, e, f, g)
{
   if (store(buffer, max_url_length) == NULL) {
      initialize();
   }
}

static_url::static_url(const url& u)
   : url()
{
   *this = u;
}

static_url::static_url(const static_url& u)
   : url()
{
   *this = u;
}

void static_url::operator = (const static_url& u)
{
   *this = (const url&)u;
}

void static_url::operator = (const url& u)
{
   scheme   = u.scheme;
   net_loc  = u.net_loc;
   port     = u.port;
   path     = u.path;
   param    = u.param;
   query    = u.query;
   fragment = u.fragment;

   if (store(buffer, max_url_length) == NULL) {
      initialize();
      return;
   }
}

int static_url::replace(const char* scheme, const char* net_loc, 
			const char* port,   const char* path, 
			const char* param,  const char* query, 
			const char* fragment)
{
   this->scheme = scheme;
   this->net_loc = net_loc;
   this->port = port;
   this->path = path;
   this->param = param;
   this->query = query;
   this->fragment = fragment;

   if (store(buffer, max_url_length) == NULL) {
      initialize();
      return -1;
   }
   
   return 0;
}

int static_url::replace(const char* start, const char* end)
{
   char tmp_buffer[MAX_URL];
   
   // check if the url is too long
   if (end - start > MAX_URL - 10)  return -1;

   // parse the url
   if (parse(start, end, tmp_buffer, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   // store the url locally
   if (store(buffer, max_url_length) == NULL) {
      initialize();
      return -1;
   }
   
   return 0;
}

int static_url::replace(const char* start, const char* end, const url& base)
{
   char tmp_buffer[MAX_URL];
   char path[MAX_URL];

   // check if the url is too long
   if (end - start > MAX_URL - 10)  return -1;

   if (parse(start, end, tmp_buffer, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   if (merge(base, path, MAX_URL) == NULL) {
      initialize();
      return -1;
   }
   if (store(buffer, max_url_length) == NULL) {
      initialize();
      return -1;
   }
   return 0;
}


ostream& operator << (ostream& os, const url& u)
{
   if (strlen(u.scheme) > 0) {
      os << u.scheme;
      os << ':';
   }
   if (strlen(u.net_loc) > 0) {
      os << "//"; 
      os << u.net_loc;
   }
   if (strlen(u.port) > 0 && strcmp(u.port, "80") != 0) {
      os << ':'; 
      os << u.port;
   }
   if (strlen(u.path) > 0) {
      os << u.path;
   }
   if (strlen(u.param) > 0) {
      os << ';'; 
      os << u.param;
   }
   if (strlen(u.query) > 0) {
      os << '?'; 
      os << u.query;
   }
   if (strlen(u.fragment) > 0) {
      os << '#'; 
      os << u.fragment;
   }

   return os;
}

istream& operator >> (istream& is, dynamic_url& u)
{
   int  count, c;
   char holder[MAX_URL]; 

   if (is.flags() & ios::skipws) {     
      is >> ws;
   }

   count = 0;
   while (count < MAX_URL - 1 && ((c = is.get()) != EOF) &&
	  (c > ' ' && c != 0x7f)) {
      holder[count++] = c;
   }
   holder[count] = 0;
   u.replace(holder, holder+count);
   
   return is;
}

istream& operator >> (istream& is, static_url& u)
{
   int  count, c;
   char holder[MAX_URL]; 

   if (is.flags() & ios::skipws) {     
      is >> ws;
   }

   count = 0;
   while (count < MAX_URL - 1 && ((c = is.get()) != EOF) &&
	  (c > ' ' && c != 0x7f)) {
      holder[count++] = c;
   }
   holder[count] = 0;
   u.replace(holder, holder+count);
   
   return is;
}
