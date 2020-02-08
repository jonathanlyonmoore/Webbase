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
/* Small program to keep reading urls from input and returning
 * their hashes
 */

#include <iostream>
#include <iomanip>
#include <string>

#include "url2hash.h"
#include "normalize.h"

inline unsigned long long loseSlash(unsigned long long hash) { 
  return (hash & 0xFFFFFFFFFFFFFFFEULL);
}

inline unsigned long long h(std::string& s, bool lose_slash) {
   if(lose_slash) return loseSlash(url2hash(s));
   else return url2hash(s);
}

int main(int argc, char *argv[])
{
  bool unsigned_flag = false;
  bool lose_slash = false;
  if(argc >= 2 && std::string(argv[1]) == "unsigned") {
    unsigned_flag = true;
  }

  if(argc >=3 && std::string(argv[2]) == "loseslash") {
    lose_slash = true;
  }

  std::string s;
  // while(std::cin >> s) {
  while(std::getline(std::cin,s)) {
    // std::cout << std::setfill('0') << std::setw(20) << url2hash(s) << std::endl;
    if(unsigned_flag) {
      std::cout << std::setfill('0') << std::setw(20) 
        << (unsigned long long) h(s, lose_slash) << std::endl << std::flush;
    }
    else {
      // setfill('0') conflicts with +/-: -12345 -> 00-12345.
      std::cout << (signed long long) h(s, lose_slash) << std::endl << std::flush;
    }
  }
}


