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
#include <iostream>
#include "url2hash.h"

inline unsigned long long loseSlash(unsigned long long hash) { 
  return (hash & 0xFFFFFFFFFFFFFFFEULL);
}

int main() {
  unsigned int docid;
  unsigned long long offset;
  unsigned long long hash;
  string url;
  while(true) {
    while(cin >> docid && cin >> offset && cin >> url) {
      hash = loseSlash(url2hash(url));
      if(hash > 0)
	cout << docid << "\t" << (signed long long) hash  << endl;
    }
    if(cin.eof()) break;
    cin.clear();
    string bad_line;
    getline(cin, bad_line);
    cerr << "bad line: " << bad_line << endl;
  }
}
