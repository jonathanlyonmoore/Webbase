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
 * linkOffsetGenerator.cc
 * Generates offsets for a binary link file
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#include <stdlib.h>

int main() {
  unsigned long long n;

  if(sizeof(n)!=8) {
    cerr << "No long long support...exiting" << endl;
    exit(1);
  }
  streampos pos = 0;

  unsigned long long source, out;
  while(true) {
    cin.read((char*) &source, sizeof(n)); // Uses endianness of Pita
    if(cin.eof()) break;
    cout << setfill('0') << setw(20) << source << '\t';
    cin.read((char*) &out, sizeof(n)); 
    cout << pos << endl;
    
    // check for failure after reading in source docId and numOutlinks
    // (either read could have caused failure)
    if(cin.fail()) {
      cerr << "input bad";
      exit(1);
    }

    // If an eof occurs after reading numOutlinks, flag an error if it
    // wasn't equal to 0
    if(cin.eof() && out!=0) { 
      cerr << "premature input eof";
      exit(1);
    }

    if(cin.fail()) abort();

    // Seek past the outlinks (n outlinks * 8 bytes/outlink)
    // Can't use seekg if input was piped
    int i;
    unsigned long long dummy;
    for(i = 0; i < out; i++) {
      cin.read((char*) &dummy, sizeof(dummy));
    }

    // check to see if the previous read succeeded
    if(cin.fail()) {
      cerr << "failure seeking past outlinks" << endl;
      cerr << "pos: " << pos << endl;
      cerr << "source: " << source;
      cerr << "out: " << out << endl;
      exit(1);
    }
    
    pos = cin.tellg();
  }
}
