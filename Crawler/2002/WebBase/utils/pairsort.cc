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
 * Sorts an edge list on the from field (used for backlink generation)
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream>
#include <fstream>
#include <iomanip>
#include <vector>
#include <algorithm>

struct Edge {
  unsigned long long from, to;
  bool operator<(const Edge& e) const {
    return from < e.from || (from == e.from && to < e.to);
  }
};

int main() {
  vector<Edge> pairs;
  
  Edge e;

  if(sizeof(e.from) != 8) {
    cerr << "No long long support...exiting" << endl;
    exit(1);
  }

  cout.fill('0');

  while(true) {
    cin.read((char*) &(e.from), sizeof(e.from)); // uses underlying endianness
    if(cin.eof()) break;
    cin.read((char*) &(e.to), sizeof(e.to));
  
    if(cin.fail()) {
      cerr << "input bad" << endl;
      exit(1);
    }

    pairs.push_back(e);
  }
  
  sort(pairs.begin(), pairs.end());

  vector<Edge>::const_iterator iter;
  for(iter = pairs.begin(); iter != pairs.end(); iter++) {
    cout.write((char*) &(iter->from), sizeof(iter->from));
    cout.write((char*) &(iter->to), sizeof(iter->to));
  }
}

		    
