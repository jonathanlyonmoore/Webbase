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
#ifndef _HTML2Plain_h_
#define _HTML2Plain_h_

class HTML2Plain {

public:
  // returns the length of the result
  inline static int convert(const char* src, char* dest)   {

    HTML2Plain p(src, dest);
    p.convertIt();
    return p.dest - p.destStart;
 } 
 // returns the length of the result
  inline static int countnon(const char* src)   {

    HTML2Plain p(src);
    return p.countNONHTML();
  } 

  static bool isLetter(int c);
  static char getCharEntity(const char* name);

protected:

  const char* src;
  const char* ptr;
  char* dest;
  char* destStart;
  int hasWhite;
  int lengthNONHTML;

  HTML2Plain(const char* src, char* dest /*, const char* delimiters = WORD_DELIMITERS*/) {

    this->src = this->ptr = src;
    this->dest = this->destStart = dest;
    hasWhite = 1;
  }

  HTML2Plain(const char* src) {   // just count
    this->src = this->ptr = src;
    lengthNONHTML = 0;
    hasWhite = 1;
  }

  int  countNONHTML();
  void convertIt();
  void skip(const char* until, int len);

  int readEntity();
  void consumeWord();
  void countWord();
  void skipDelimiters();
  void skipTagging();
  void end();
};



#endif
