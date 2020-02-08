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
#include "HTML2Plain.h"
#include "Utils.h"

//#include <string>

inline char HTML2Plain::getCharEntity(const char* name) {

  char c = Utils::translate((char*)name);
  //  cout << "--entity: " << name << " translated into " << c << endl;
  
  return c;
}


// inline void HTML2Plain::write(int c) {

//   if(c == ' ' || (c > 0 && c <= 13) || c == '\t') {
//     if(!hasWhite) {
//       hasWhite = 1;
//       *dest++ = ' ';
//     }
//   } else {
//     hasWhite = 0;
//     *dest++ = c;
//   }
// }

inline void HTML2Plain::end() {

  ptr += strlen(ptr);
}

inline void HTML2Plain::skip(const char* needle, int len) {

  const char* p;
  if((p = strstr(ptr, needle)) == NULL)
    end();
  else
    ptr = p + len;
}

inline int HTML2Plain::readEntity() {

  int c = *ptr;

  if(c == '#' || isLetter(c)) {
    char* entityEnd;
    if((entityEnd = strchr(ptr, ';')) == NULL) {
      end();
      return -1;
    } else {
      *entityEnd = '\0';
      c = getCharEntity(ptr);
      *entityEnd = ';';
      ptr = entityEnd + 1;
      return c;
    }
  } else
    return '&';
}

// inline int HTML2Plain::getNextOf(const char * delim) {

//   if((ptr = strpbrk(ptr, delim)) == NULL) {
//     ptr = end;
//     return -1;
//   } else
//     return *ptr++;
// }

// inline void HTML2Plain::writeWord() {

//   int len;
//   if((len = (ptr - wordStart)) > 0) {
//     memcpy(dest, wordStart, len);
//     // put a space after the word
//     dest += len;
//     *dest++ = ' ';
//     wordStart = ptr + 1; // FIXME
//   }
// }

inline bool HTML2Plain::isLetter(int c) {

  return c >= (int)'A' || (c >= (int)'0' && c <= (int)'9');
}

inline void HTML2Plain::skipDelimiters() {

  int c;
  while((c = *ptr) != '\0') {
    if(isLetter(c) || c == '<' || c == '&')
      return;
    ptr++;
  }
}

inline void HTML2Plain::consumeWord() {

  const char* start = ptr;
  const char* olddest = dest;

  while(true) {

    int c = *ptr;

    switch(c) {
      
    case '&':
      int len;
      if((len = (ptr - start)) > 0) {
	memcpy(dest, start, len);
	dest += len;
      }
      ptr++;
      c = readEntity();
      *dest++ = c;
      start = ptr;
      break;
      
    case '\0':
      goto stoploop;

    default:
      if(isLetter(c))
	ptr++;
      else
	goto stoploop;
    }
  }

 stoploop:

  int len;
  if((len = (ptr - start)) > 0) {
    memcpy(dest, start, len);
    // put a space after the word
    dest += len;
  }
  if(dest > olddest)
    *dest++ = ' ';
}

inline void HTML2Plain::countWord() {

  const char* start = ptr;
 
  while(true) {

    int c = *ptr;
    
    switch(c) {
      
    case '&':
      int len;
      if((len = (ptr - start)) > 0) {
	lengthNONHTML += len; 
      }
      ptr++;
      c = readEntity();
      //lengthNONHTML += strlen(c); 
      
      start = ptr;
      break;
      
    case '\0':
      goto stoploop;
      
    default:
      if(isLetter(c)){
	ptr++;
	lengthNONHTML ++; // added 10/1 gsw
	//cout << lengthNONHTML << "|";
      }
      else
	goto stoploop;
    }
  }

 stoploop:
  ptr++;///////////////////////////////ptr--;  removed 10/1 gsw 
}

inline void HTML2Plain::skipTagging() {

  if(*ptr != '<')
    return;

  ptr++;

//   const char* saved = ptr++;

  int c = *ptr++;
  switch(c) {

  case '!':

    c = *ptr++;
    switch(c) {
    case '-':
      skip("->", 2);
      return;
    case '\0':
      ptr--;
      return;
    default:
      skip(">", 1);
      return;
    }

  case '\0':
    ptr--; // restore
    return;
      
  default:
    skip(">", 1);
    return;
  }
  // undo
  //  ptr = saved;
}

void HTML2Plain::convertIt() {

  while(*ptr != '\0') {

    skipTagging();
    skipDelimiters();
    consumeWord();
  }
  if(dest > destStart)
    *(--dest) = '\0'; // remove extra space
  else
    *dest = '\0'; // terminate it
 }

int HTML2Plain::countNONHTML() {

  while(*ptr != '\0') {
    skipTagging();
    skipDelimiters();
    countWord();
  }
  return lengthNONHTML;
}


/*
int
main(int argc, char* argv[]) {

  char res[256];

  HTML2Plain::convert(argv[1], res);

  cout << "Result: '" << res << "'" << endl;
}
// test strings:
// ' <tag>word</tag> (word)--here\t<\!bad>more<\!--com-ment-->text &amp;&auml;&#65;end '
*/
