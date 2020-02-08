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
 * blinkgen.cc
 *
 * Takes a forward link bucket (binary list form) and generates
 * a set of binary backlink buckets (pair form).  Filter the buckets through
 * sort and listConvert to get the final format (backlinks in binary list form)
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>
#include <vector.h>
#include <assert.h>
#include <strstream.h>

#include "blinkgen.h"

#define ASSERT assert

// belongs in a header file
#define BASE_FILENAME "blinks"

BLinkGen::BLinkGen(const char* blinksdir, const char* baseFilename,
		   bool append0) {
  append = append0;
  if(!createFileTable(blinksdir, baseFilename)) {
    cerr << "Exiting..." << endl;
    exit(1);
  }
}

bool
BLinkGen::createFileTable(const char* linksdir, const char* baseFilename) {
  // now open the corresponding files
  // buf is 2*MAX_LINE+2 so it can hold
  //             directory, optional /, filebase, and 2 digit suffix
  int i;
  for(i = 0; i < 64; i++) {
    char buf[2*MAX_LINE+2];

    ostrstream fname(buf, 2*MAX_LINE+2);
    fname << linksdir;
    if(linksdir[strlen(linksdir)-1] != '/') {
      fname << '/';
    }
    fname << baseFilename << setfill('0') << setw(2) << i << ends;
    if (append) {
      cout << "Appending to file: " << buf << endl;
      files[i].open(buf, ios::app);
    }
    else {
      cout << "Opening file: " << buf << endl;
      files[i].open(buf);
    }

    if(!files[i].is_open()) {
      cerr << "Could not open file: " << buf << endl;
      return false;
    }
  }
  return true;
}

bool
BLinkGen::flipLink(unsigned long long sourceId, 
		       vector<unsigned long long int>& destIds) {
  vector<unsigned long long int>::iterator iter;

  for(iter = destIds.begin(); iter != destIds.end(); iter++) {
    ostream& file = getFileFor(*iter);

    file.write((char*) &(*iter), sizeof(*iter));
    file.write((char*) &sourceId, sizeof(sourceId));
    if(file.fail()) {
      cerr << "Couldn't write link" << endl;
      return false;
    }

  }
  
  return true;
}

ofstream&
BLinkGen::getFileFor(unsigned long long id) {
  int whichFile = id >> 58;
  ASSERT(whichFile < 64 && whichFile >= 0);
  return  files[whichFile];
}

// The driver for BLinkGen
int main(int argc, char** argv) {
  if(argc != 2 && argc != 3) {
    cerr << "Usage: blinkgen dir [a]\nSpecify 'a' for append mode" << endl;
    exit(1);
  }

  bool append = (argc == 3 && *argv[2]=='a');
    
  BLinkGen bgen(argv[1], BASE_FILENAME, append);
  
  unsigned long long sId, num;
  while(true) {
    cin.read((char*) &sId, sizeof(sId));
    if(cin.eof()) break;
    
    if(cin.fail()) {
      cerr << "input bad" << endl;
      exit(1);
    }

    cin.read((char*) &num, sizeof(num));

    if(cin.fail()) {
      cerr << "input bad" << endl;
      exit(1);
    }
    
    vector<unsigned long long> destIds;
    unsigned long long i, dId;
    for(i = 0; i < num; i++) {
      if(cin.eof()) {
	cerr << "premature eof" << endl;
	exit(1);
      }
      cin.read((char*) &dId, sizeof(dId));
      
      destIds.push_back(dId);
    }

    if(cin.fail()) {
      cerr << "input failure" << endl;
      exit(1);
    }
    
    bgen.flipLink(sId, destIds);
    
  }
}
    
