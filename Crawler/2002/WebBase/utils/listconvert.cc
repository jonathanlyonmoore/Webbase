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
/*
 * listconvert.cc
 * Convert links represented by an edge list into an adjacency list
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <vector.h>

void print(unsigned long long source, vector<unsigned long long> dests);

int main() {
  bool first = true;
  unsigned long long a, b, curr=0;

  vector<unsigned long long> dests;

  if(sizeof(unsigned long long) != 8) {
    cerr << "No unsigned long long support.  Aborting." << endl;
    exit(1);
  }

  while(true) {
    cin.read((char*) &a, sizeof(a));
    if(cin.eof()) break;
    cin.read((char*) &b, sizeof(b));
    if(cin.fail()) {
      cerr << "Input failure." << endl;
      exit(1);
    }

    if(a != curr || first) {
      if(!first) print(curr, dests);
      else first = false;
      curr = a;
      dests.erase(dests.begin(), dests.end());
    }

    dests.push_back(b);
  }
  print(curr, dests);
}

void print(unsigned long long source, vector<unsigned long long> dests) {
  cout.write((char*) &source, sizeof(source));
  unsigned long long size = dests.size();
  cout.write((char*) &size, sizeof(size));
  vector<unsigned long long>::iterator iter;
  for(iter = dests.begin(); iter != dests.end(); iter++) {
    cout.write((char*) &*iter, sizeof(*iter));
  }
}

