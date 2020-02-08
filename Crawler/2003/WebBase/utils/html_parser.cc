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
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <iostream>
#include "html_parser.h"


const char *html_parser::tag_string[]
    = { "area", "a", "base", "link", "frame", "iframe", "img" };
const int html_parser::tag_length[]
    = { 4, 1, 4, 4, 5, 6, 3 };
const int html_parser::tag_token[]
    = { AREA, A, BASE, LINK, FRAME, IFRAME, IMG };

      
int html_parser::start_parse(const char* start, const char* end)
{
   buffer_start = start;
   buffer_end   = end;
   buffer_pointer = start;
   
   return 0;
}

int html_parser::end_parse()
{
   buffer_start = NULL;
   buffer_end   = NULL;
   buffer_pointer = NULL;
   
   return 0;
}

int html_parser::get_link(const char*& start, const char*& end)
{
   const char *s, *d;
   int        tag;
   
   while (find_tag(s, d) >= 0) {
      tag = find_url(s, d);
      if (tag >= 0) {
	 start = s;
	 end   = d;
	 return tag;
      }
   }
   return -1;
} 


inline char html_parser::to_lower(char c)
{
   if ('A' <= c && c <= 'Z') {
      c +=  ('a' - 'A');
   }
   return c;
}

inline char html_parser::normalize_ws(char c)
{
   if (isspace(c)) {
      return ' ';
   }
   return c;
}

const char* html_parser::find_substr(const char* begin, const char* end, 
			       const char* substr)
{
   register const char *s, *d;

   if (begin >= end)  return NULL;

   s = begin;
   d = substr;
   while (s < end && *d != 0) {
      if (to_lower(*s) == to_lower(*d)) {
	 d++;
      } else {
	 s -= d - substr;
	 d = substr;
      }
      s++;
   }
   if (*d == 0) return s;

   return NULL;
}

int html_parser::find_tag(const char*& start, const char*& end)
{
   register const char* s = buffer_pointer;
   register char  c;

   if (buffer_pointer >= buffer_end) return -1;

/* revise to search only for tags from outside SGML comments
   see below do{...}while(1) loop.  -- wlam, 3 Sep 2003
   
   // find the beginning of a tag
   while (s < buffer_end && (c = *s) != '<') s++;
   if (s >= buffer_end)  return -1;
   start = s + 1;
*/
   do {
      while (s < buffer_end && (*s != '<')) ++s;
      if (s >= buffer_end)  return -1;

      // We'd like to skip over HTML comments <!-- ... -- -- ... -- >, but
      // rules defining HTML comments (SGML) are widely misunderstood, 
      // and probably still misinterpreted.
      // Further, some interpretations are directly contradictory, 
      // so no matter what we do, we will always find the same end-of-comment
      // authors intend us to find.
      // Since we lose no matter what, at least we could be correct about it.

      // So far as I can tell, "correct" is (as described in HTML 2.0/RFC 1866)
      // < ! ( - - (string not containing --)* - - (whitespace)* )* >
      // HTML 3.2 is silent on comments, and HTML 4.01 specifies a more
      // restrictive                                             ){1} >
      // and declaring that authors "should" avoid -- inside a comment.
      // So, HTML4 explicitly describes a subset of HTML2 comments, but does
      // not explicitly contradict HTML2.

      // If the apparent HTML comment includes non-comment material, we pass
      // the entire element ("tag") out for interpretation.
      // Because we cannot modify our buffer, and must return pointers into it,
      // we return the entire element, comments included, and 
      // require the caller to parse out the comments again.  (Unfortunate...)

      if ((s+1 < buffer_end) && (s+2 < buffer_end) &&
          (s[1]=='!') && (s[2]=='>')) {
         s += 3; continue;  // <!> is a zero-comment HTML comment
      }

      if ((s+1 < buffer_end) && (s+2 < buffer_end) && (s+3 < buffer_end) &&
          (s[1]=='!') && (s[2]=='-') && (s[3]=='-')) {

         // Found beginning of first HTML comment <!--
         const char *comment_s = s;
         bool revert = false, done = false;
         s += 4;

         while (!done) {
            // Find end of this comment --
            while ((s < buffer_end) && (*s != '-') &&
                   (s+1 < buffer_end) && (s[1] != '-'))   ++s;
            if ((s >= buffer_end) || (s+1 >= buffer_end))   return -1;
            s += 2;

            // Absorb trailing whitespace
            while ((s < buffer_end) && isspace(*s)) ++s;

            // Finish HTML comment element if >
            if (*s == '>') { done = true;  break; }

            // Else, next HTML comment must start --
            if ((*s != '-') || (s+1 >= buffer_end) || (s[1] != '-')) {
               revert = true;  done = true;  break;
            }
            s += 2;
         }
         if (revert) {
            s = comment_s;  break;
         } else {
            continue;
         }
      } else {
         // Maybe it really is an element
         break;
      }
   } while (1);
   start = s + 1;

   // find the end of a tag
   while (++s < buffer_end && (c = *s) != '>') {
      /*
      if (c == '\"') {
	 while (++s < buffer_end && (c = *s) != '\"');
	 if (s >= buffer_end) return -1;
      } else if (c == '\'') {
	 s++;
	 while (++s < buffer_end && (c = *s) != '\'');
	 if (s >= buffer_end) return -1;
      }
      */
   }
   if (s >= buffer_end)  return -1;
   end = s;

   // adjust buffer pointer
   buffer_pointer = s + 1;

   return 0;
}


int html_parser::find_url(const char*& start, const char*& end)
{
   register char c;
   register const char *s = start;
   int  i;
   int  tag_found;
   
   // Check wheter tag is long enough.
   // The length of a tag should be at least '8' to contain url.
   //   minimal case : <A HREF=.>
   if (end - start < 8)  return -1;

   // check whether it is appropriate tag
   tag_found = 0;
   for (i = 0; i < (int)(sizeof(tag_string)/sizeof(const char*)); i++) {
      if (strncasecmp(s, tag_string[i], tag_length[i]) == 0) {
	 tag_found = tag_token[i];
	 s += tag_length[i];
	 break;
      }
   }
   if (tag_found == 0 || s >= end)   return -1;
   c = normalize_ws(*s++);
   if (c != ' ' || s >= end)  return -1;

   //   cerr << "found tag: " << tag_found << endl;

   // find the href/src attributes
   if (tag_found == A || tag_found == AREA || tag_found == LINK ||
       tag_found == BASE) {
      s = find_substr(s, end, "href");
   } else if (tag_found == FRAME || tag_found == IFRAME || 
	      tag_found == IMG) {
      s = find_substr(s, end, "src");
   }
   if (s == NULL)  return -1;
      
   while (s < end && (c = normalize_ws(*s)) == ' ') s++;
   if (s >= end)  return -1;
   if ((c = *s++) != '=')  return -1;
   while (s < end && (c = normalize_ws(*s)) == ' ') s++;
   if (s >= end)  return -1;


   // url starts!
   register char end_char = ' ';
   c = *s;
   if (c == '\'' || c == '\"') {
      end_char = c;
      ++s;
   }

   start = s;
   while (s < end && (c = normalize_ws(*s)) != end_char) s++;
   end = s;

   return tag_found;
}
	 
	  
      
