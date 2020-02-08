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
/**************************************************************************
 * Small utility script to parse a url and dump out the individual 
 * components. Uses the parsing code in Url.cc to accomplish the task.
 * Build as   g++ -o urlparser UrlParser.cc Url.cc
 *
 * Author:  Sriram Raghavan <rsram@cs.stanford.edu>
 **************************************************************************/  

#include <iostream>
#include <string>
#include "Url.h"

int main(int argc, char *argv[])
{
  if (argc != 2) {
    cout << "Usage: " << argv[0] << " <uri> " << endl;
    return(-1);
  }
  string inputUrl(argv[1]);
  Url::ParsedUrl url(inputUrl);
  
  cout << "Scheme: " << url.getScheme() << endl;
  cout << "Params: " << url.getParams() << endl;
  cout << "Query: " << url.getQueryString() << endl;

  Url::NVMap nvmap = url.getQueryParameters();
  Url::NVMap::iterator iter = nvmap.begin();

  cout << "Parsing out query parameters: " << endl;
  while (iter != nvmap.end()) {
    cout << "\t" << iter->first << " = " << iter->second << endl;
    iter++;
  }

  Url::NetworkLocation nl = url.getNetworkLocation();
  cout << "Host: " << nl.getHost() << endl;
  cout << "Port: " << nl.getPort() << endl;
  cout << "Username: " << nl.getUser() << endl;
  cout << "Password: " << nl.getPassword() << endl;

  Url::UrlPath up = url.getPath();
  cout << "Path components: " << up.getPathName() << endl;

  return(1);
}
