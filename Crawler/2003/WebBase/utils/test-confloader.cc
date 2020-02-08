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
   test-confloader.cc - lame test utility for confloader.cc
   Wang Lam <wlam@cs.stanford.edu>
   August 2001

   Based on test-confloader.pl.
*/

#include "confloader.h"

int main(int argc, char *argv[])
{
   if (argc < 2) cout << "No file specified." << endl;
   
   ConfLoader cf;
   cf.loadValues(argv[1],'=');

   string s;
   while (cin >> s) {
      cout << s << " = " << cf.getValue(s.c_str()) << endl;
   } 
   return 0;
}

