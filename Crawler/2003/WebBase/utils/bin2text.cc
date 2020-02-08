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
 * bin2text.cc
 * Converts a binary input stream of numbers to an ASCII output stream of
 * numbers
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <fstream.h>
#include <iomanip.h>
#include <stdlib.h>
#include "Utils.h"

int main() {
  unsigned long long a, b;

  if(sizeof(a) != 8) {
    cerr << "No long long support...exiting" << endl;
    exit(1);
  }

  cout.fill('0');

  while(true) {
    cin.read((char*) &a, sizeof(a));
    //    a = Utils::ntohll(a);
    if(cin.eof()) break;
    cin.read((char*) &b, sizeof(b));
    //    b = Utils::ntohll(b);
  
    if(cin.fail()) {
      cerr << "input bad" << endl;
      exit(1);
    }

    cout << setw(20) << a << '\t' << setw(20) << b << endl;
  }
}


