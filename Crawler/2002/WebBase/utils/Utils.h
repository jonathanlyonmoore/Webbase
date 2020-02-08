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
/*********************************************************************************
 *                        Header file for Utils class
 * 
 * Author : Sriram Raghavan (rsram@cs.stanford.edu)
 *********************************************************************************/

#ifndef _Utils_h_
#define _Utils_h_

#include <hash_map>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <iostream.h>
#include <sys/time.h>
#include <pthread.h>

struct str_equal  {
  bool operator()(const char* s1, const char* s2) const {
    return (strcmp(s1, s2) == 0);
  }
};

typedef struct
{
  const char *entity;
  unsigned char	equiv;
} entityPair;

static const int ENDIANESS_DETECTOR = 1;

class Utils {

public : 
  static bool readn(int fd, char *buf, int nbytes);
  static bool writen(int fd, char *buf, int nbytes);
  static char* writeNumber(char* dest, const char *src, int len);
  static const char* readNumber(const char* src, char *dest, int len);
  static unsigned long long ntohll(unsigned long long);
  static unsigned long long htonll(unsigned long long);

  static bool isLittleEndian();
  static long long currentTimeMillis();
  static void threadSleep(long long millis);
  static const char *skipHeader(const char *p);
  static unsigned char translate(char *entity);
  static bool reverseServer(char *);
};

inline const char* Utils::readNumber(const char* src, char *dest, int len) {

  if(isLittleEndian()) {
    dest += len - 1;
    while(len-- > 0)
      *(dest--) = *(src++);
  } else {
    memcpy(dest, src, len);
    src += len;
  }

  return src;
}

inline bool Utils::isLittleEndian() {

  return *(char*)&ENDIANESS_DETECTOR;
}

inline char* Utils::writeNumber(char* dest, const char *src, int len) {

  // FIXME: we need big endian, otherwise poor compression (prefix would not match!)
  
  if(isLittleEndian()) {
    src += len - 1;
    while(len-- > 0)
      *(dest++) = *(src--);
  } else {
    memcpy(dest, src, len);
    dest += len;
  }

  return dest;
}

inline unsigned long long Utils::htonll(unsigned long long hostlonglong) {

  if(isLittleEndian()) {
    unsigned long long netlonglong;
    char* src = (char*)&hostlonglong;
    char* dest = (char*)&netlonglong;
    int len = sizeof(hostlonglong);
    dest += len - 1;
    while(len-- > 0)
      *(dest--) = *(src++);
    return netlonglong;
  } else {
    return hostlonglong;
  }
}

inline unsigned long long Utils::ntohll(unsigned long long netlonglong) {
  return htonll(netlonglong);
}

#endif
