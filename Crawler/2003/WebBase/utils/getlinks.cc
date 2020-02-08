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
 * getlinks.cc
 * 
 * given an offset, this function will return the appropriate link
 *
 * Author: Taher H. Haveliwala
 */

#include <iostream.h>
#include <fstream.h>
#include <vector.h>
#include <iomanip.h>

void error() {
  cerr << "Failure reading links file" << endl;
  exit(1);
}

// given 'offset', stores a vector of forward links in dests
// the source docId is stored in 'source' 
void
getLinks(istream& input, unsigned offset, 
	 unsigned long long &source,
	 vector<unsigned long long> &dests) {
  input.seekg(offset, ios::beg);
  
  unsigned long long num;

  input.read((char*) &source, sizeof(source));
  if(input.eof()) error();
  input.read((char*) &num, sizeof(num));
  if(input.fail()) error();

  if(input.eof() && num!=0) error();

  unsigned long long i;
  unsigned long long dest;
  for(i = 0; i < num; i++) {
    if(input.eof()) error();
    
    input.read((char*) &dest, sizeof(dest));
    
    dests.push_back(dest);
  }
}

// given the linksfile and the offset, will print out the link information
// note: to get the name of the linksfile, the DocId's top 6 bits must
// be used to form the filename: links/linksxx where xx is the value of the
// top 6 bits
int main(int argc, char** argv) {
  if(argc != 3) {
    cerr << "Usage: getlinks linksfile offset" << endl;
    exit(1);
  }

  const char* filename = argv[1];
  unsigned offset = atol(argv[2]);

  unsigned long long source;
  vector<unsigned long long> dests;

  ifstream file(filename);

  getLinks(file, offset, source, dests);

  cout << setfill('0') << setw(20) << source << "\t" 
       << setfill('0') << setw(20) << dests.size();

  vector<unsigned long long>::iterator iter;
  for(iter = dests.begin(); iter != dests.end(); iter++) {
    cout << " " << setfill('0') << setw(20) << *iter;
  }
  cout << endl;
}



