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
#include <fstream.h>
#include "net_mod.h"

main(int argc, char* argv[])
{
   net_mod* nm = new net_mod(net_mod::BLOCKING);
   ifstream  is(argv[1]);
   is.unsetf(ios::skipws);
  
   string request;

   // read file
   request = "NEW_SITES\r\n";


   copy(istream_iterator<char>(is), istream_iterator<char>(), 
	back_inserter(request));
   request += "\r\n\r\n";

   // get the server address
   vector<string>   name;
   vector<unsigned> ip;
   net_addr  addr;
   {
      net_mod::resolve_name(string(argv[2]), ip, name);
      addr.ip = ip[0];
      addr.port = htons(atoi(argv[3]));
   }

   // connect to the server
   int   sd, size;
   sd = nm->connect(addr);
   nm->write(sd, request.data(), request.size());
   /*
   nm->set_read(sd);
   size = nm->read(sd, buffer, 1024*1024);
   buffer[size] = 0;
   cout << buffer;
   */
   nm->close(sd);

}
   
   
   
   
