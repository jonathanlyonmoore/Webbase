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
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/times.h>
#include <limits.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>
#include <fstream.h>
#include "my_utility.h"

main(int argc, const       char **argv)
{
   struct tms dummy;
   clock_t  then, now;

   if (argc != 3) {
      cerr << "usage: " << argv[0] << " input output" << endl;
      exit (1);
   }
   
   ifstream input(argv[1]);
   if (!input) {
      cerr << "Cannot open input file\n";
      exit (1);
   }
   ofstream output(argv[2]);
   if (!output) {
      cerr << "Cannot open output file\n";
      exit(1);
   }

   struct hostent *hp;

   lower_string site, alias;
   int  count;

   while (input) {
      // starting time
      then = times(&dummy);

      // read a site name
      input >> site;
      
      // log
      //cerr << site.c_str() << '(' << site.length() << ')' << endl;
      
      hp = gethostbyname(site.c_str());
      if (hp == NULL) {
	 //cerr << "  Ignored" << endl;
	 continue;
      }

      // print site name
      output << site << ' ';

      // print ip list
      for (char **p = hp->h_addr_list; *p != 0; p++) {
	 struct in_addr in;
	 memcpy(&in.s_addr, *p, sizeof (in.s_addr));
	 output << inet_ntoa(in) << ' ';
      }

      // print name list
      alias = hp->h_name;
      if (site.compare(alias) != 0) {
	 output << alias << ' ';
      }
      for (char **q =   hp->h_aliases; *q != 0; q++) {
	 alias = *q;
	 if (site.compare(alias) != 0) {
	    output << alias << ' ';
	 }
      }
      output << endl;

      // log
      if (++count % 1000 == 0) {
	 output << flush;
	 cerr << count << " sites have been processed...\n" << flush;
      }
      
      // interval between requests >= 1 sec
      now = times(&dummy);
      while ((now - then) < CLK_TCK) {
	 struct timeval tv;
	 int diff;

	 diff = (now - then)*1000/CLK_TCK;
	 tv.tv_usec = diff % 1000;
	 tv.tv_sec  = diff / 1000;
	 select(0, NULL, NULL, NULL, &tv);
	 //cerr << "Now: " << now << " Then: " << then << endl << flush;

	 now = times(&dummy);
      }
   }
   
   cerr << "Done!\n" << flush;
   exit (0);
}
