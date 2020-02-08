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
 * text2bin.cc
 * Converts a stream of whitespace separated ascii numbers into a 
 * stream of consecutive 8-byte (unsigned long long) integers
 *
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#include <iostream.h>
#include <stdlib.h>
#include "Utils.h"

int main() {
  unsigned long long n;

  if(sizeof(n) != 8) {
    cerr << "No long long support...exiting." << endl;
    exit(1);
  }

  while(cin >> n) {
    //    n = Utils::htonll(n);
    cout.write((char*) &n, sizeof(n)); 
  }
}
