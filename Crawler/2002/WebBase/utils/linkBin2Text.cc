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
 * linkBin2Text.cc
 * Converts binary links files into ascii links files
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#include <stdlib.h>
#include <vector>

int main() {
  unsigned long long i, k;

  unsigned long long src, dst;

  unsigned int cntr = 0;

  if(sizeof(i)!=8) {
    cerr << "No long long support...exiting" << endl;
    exit(1);
  }
  
  cout.fill('0');
  vector<unsigned long long int> v;

  while(true) {
    v.clear();
    cin.read((char*) &src, sizeof(src)); // Uses endianness of Pita
    if(cin.eof()) break;
    if(src == 0) {
      cerr << "src was zero in lb2t" << endl;
    }
    //    cout << setw(20) << n;
    cin.read((char*) &k, sizeof(k)); 
    //    cout << "\t" << setw(20) << k;

    // check for failure after reading in source docId and numOutlinks
    // (either read could have caused failure)
    if(cin.fail()) {
      cerr << "input bad" << endl;
      exit(1);
    }

    // If an eof occurs after reading numOutlinks, flag an error if it
    // wasn't equal to 0
    if(cin.eof() && k!=0) { 
      cerr << "premature input eof" << endl;
      exit(1);
    }

    // Loop through the outlinks
    for(i = 0; i < k; i++) {
      if(cin.eof()) {
	cerr << "premature eof" << endl;
	exit(1);
      }
      cin.read((char*) &dst, sizeof(dst)); 
      //      cout << " " << setw(20) << n;
      if(dst != 0) v.push_back(dst);
    }
    // check to see if the previous k reads succeeded
    if(cin.fail()) {
      cerr << "input failure" << endl;
      exit(1);
    }
    //    cout << endl;

    if(src == 0 || v.size() == 0 || v.size() > 256) continue;

    if(++cntr % 1000 == 0) {
      cerr << "Processed: " << cntr << " entries." << endl;
    }

    cout << setw(20) << src;
    cout << "\t" << v.size();
    vector<unsigned long long int>::iterator iter;
    for(iter = v.begin(); iter != v.end(); iter++) {
      cout << " " << *iter;
    }
    cout << endl;
  }
}



