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
//
// wcHTML2Plain is based on WebBase/utils/HTML2Plain, but treats tags a differently.
// Usually when there is a tag, it is replaced with a space but the tags enumerated 
// in nodivArray should not be replaced by a space.
//


#ifndef _WCHTML2PLAIN_h_
#define _WCHTML2PLAIN_h_

#include "stringHash.h"

class NoDivTags : public stringHashSet {
  
  public:
    const static char *nodivArray[];
    const static int   MAXARRAY;
    void init() {
        for (int i = 0; i < MAXARRAY; i++) {
            insert(nodivArray[i]);
        }
    }  
};
    
class wcHTML2Plain {
    
  public:

    static NoDivTags *noDivTags;
    
    inline static int convert(const char* src, char* dest) {
        wcHTML2Plain p(src, dest);

        if (noDivTags == NULL) {
            noDivTags = new NoDivTags();
            noDivTags->init();
        }

        p.convertIt();
        return p.dest - p.destStart;
    } 
    
    static bool isLetter(int c);
    static char getCharEntity(const char* name);
    
  protected:
    const char* src;
    const char* ptr;
    char* dest;
    char* destStart;
    
    wcHTML2Plain(const char* src, char* dest) {
        this->src = this->ptr = src;
        this->dest = this->destStart = dest;
    }
    
    void convertIt();
    void skip(const char* until, int len);
    
    int readEntity();
    void consumeWord();
    void skipDelimiters();
    void skipTagging();
    bool isDivTag(const char *tag);
    const char *skipToTagBegin(const char *tag);
    void copyToTagEnd(const char *tag, char *newTag, char *endTag);
    void end();
};


#endif

