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
#include <iostream.h>
#include <fstream.h>
#include <string>
#include <vector>
#include "my_utility.h"
#include "url.h"
#include "manager.h"

FILE* tmp_log;

main(int argc, char *argv[])
{
   /*
   const int maxline = 100;
   char line[maxline];

   cout << argv[1] << endl;

   ifstream in(argv[1]);
   if (!in) {
      cerr << "cannot open input file" << endl;
      exit(1);
   }

   in.get(line, maxline, '\n');
   cout << line;

   in.get(line, maxline, '\n');
   cout << line;

   */

   /*
   const int maxline = 30;
   char line[maxline];

   ifstream in(argv[1]);
   if (!in) {
      cerr << "Cannot open input file" << endl;
      exit(1);
   }

   while (in) {
      readline(in, line, maxline);
      cout << line << endl;
   }
   */

   /*
   bit_vector v, v2;
   int  count;
   ofstream os(argv[1]);
   ifstream is(argv[1]);
   
   const int vector_size = 1000;
   v.reserve(vector_size);
   for (int i = 0; i < vector_size; i++) {
      v.push_back(false);
   }

   cout.rdbuf()->sputc('T');


   cout << "Type in count : ";
   cin >> count;

   for (int i = 0; i < count; i++) {
      int b;

      cout << "Type in url " << i+1 << " : ";
      cin >> b;
      
      v[b] = true;
   }

   cout << "User input" << endl;
   cout << "Size: " << v.size() << endl;
   for (int i = 0; i < (int)v.size(); i++) {
      if (v[i])  cout << i << endl;
   }

   writelist(os, "BitVector", v);
   os << flush;

   readlist(is, "BitVector", v2);
   
   cout << "After save/load" << endl;
   cout << "Size: " << v2.size() << endl;
   for (int i = 0; i < (int)v2.size(); i++) {
      if (v2[i])  cout << i << endl;
   }

   writelist(cout, "BitVector", v2);
   */

   manager::queue_info* qi = new manager::queue_info;
   char *tst = NULL;
   
   ofstream os(argv[2]);
   if (!os) {
      cerr << "Cannot open output file\n";
      exit(1);
   }

   for(int i = 0; i < 3; i++) {
      ifstream is(argv[1]);
      if (!is) {
	 cerr << "Cannot open input file\n";
	 exit(1);
      }
      qi->load(is);
   }

   qi->save(os);
}
