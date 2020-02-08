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
 * their normalized versions - has to be compiled along with the
 * the normalize function defined in normalize.cc
 * Author : Sriram Raghavan - rsram@cs.stanford.edu 
 */

#include <iostream.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "normalize.h"

int main(int argc, char *argv[])
{
  char buff[MAX_LINE_LENGTH];

  string s;
  while (getline(cin, s)) {
    buff[s.copy(buff, MAX_LINE_LENGTH-1)] = '\0';
    if (!  normalize(buff)) 
      cerr << "Incorrect format - cannot be normalized" << endl;
    else cout << buff << endl;
  }
}


