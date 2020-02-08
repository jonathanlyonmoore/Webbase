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
#include "wcHTML2Plain.h"
#include "Utils.h"

const char *NoDivTags::nodivArray[] =  {"a", "b", "big", "em", "font", "i", "s", "small", "strike", "strong", "sub", "sup", "tt", "u", "code", "dfn", "kbd", "samp"};

const int NoDivTags::MAXARRAY = (int)(sizeof(nodivArray)/sizeof(const char*));
    
NoDivTags *wcHTML2Plain::noDivTags = NULL;

inline char wcHTML2Plain::getCharEntity(const char* name) 
{
    char c = Utils::translate((char*)name);
    return c;
}

inline void wcHTML2Plain::end() 
{
    ptr += strlen(ptr);
}

inline void wcHTML2Plain::skip(const char* needle, int len) 
{
    const char* p;
    if((p = strstr(ptr, needle)) == NULL)
        end();
    else
        ptr = p + len;
}

inline int wcHTML2Plain::readEntity() 
{
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

inline bool wcHTML2Plain::isLetter(int c) 
{
    return c >= (int)'A' || (c >= (int)'0' && c <= (int)'9');
}

inline void wcHTML2Plain::skipDelimiters() 
{
    int c;
    while((c = *ptr) != '\0') {
        if (isLetter(c) || c == '<' || c == '&')
            return;
        ptr++;
        if (dest != destStart && *(dest-1) != ' ') {
            *dest++ = ' ';
        }
    }
}

inline void wcHTML2Plain::consumeWord() 
{
    const char* start = ptr;

    while (true) {
        
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
}

inline void wcHTML2Plain::skipTagging() 
{
    if(*ptr != '<')
        return;
    
    ptr++;

    const char *beginTag = ptr;

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
        
        if (isDivTag(beginTag) && dest != destStart && *(dest-1) != ' ') {
            *dest++ = ' ';
        }

        return;
    }
}

inline bool wcHTML2Plain::isDivTag(const char *start)
{
    char tag[16];
    const char *beginTag = skipToTagBegin(start);

    copyToTagEnd(beginTag, tag, (char *)(tag + 15));

    NoDivTags::const_iterator it = noDivTags->find(tag);

    return (it == noDivTags->end()); 
}

inline const char *wcHTML2Plain::skipToTagBegin(const char *tag) 
{
    int c;
    while((c = *tag) != '\0') {
        if (isLetter(c))
            return tag;
        tag++;
    }
    return tag;
}

inline void wcHTML2Plain::copyToTagEnd(const char *tag, char *newTag, char *endTag) 
{
    int c;
    while(true) {
        if (isLetter(c = *tag) && newTag < endTag) {
            *newTag = isupper(c) ? tolower(c) : c;
        } else {
            *newTag = '\0';
            return;
        }
        tag++;
        newTag++;
    }
}

void wcHTML2Plain::convertIt() 
{
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


