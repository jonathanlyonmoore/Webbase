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
#include <cctype>
#include "mygetline.h"
#include "trim.h"
#include "readmsg.h"

using std::string;

int readmsg(std::istream& sin, vec_msgtype& msg)
{
   int     count = 0;
   string  line;
   string  key, value;
   string::size_type  keyend;

   while (mygetline(sin, line)) {
      line = trim(line);
      if (line.size() == 0) break;

      // a line starting with '#' is a comment line
      if (*line.begin() == '#')  continue;

      // a line without ':' is a malformed line
      if ((keyend = line.find(':')) == string::npos) break;

      // extract the key
      key = trim(line.substr(0, keyend));
      if (key.size() == 0) break;

      // extract the value
      keyend++;
      value = trim(line.substr(keyend, line.size()-keyend));
      if (value.size() == 0) break;

      // insert the (key, value) pair
      msg.push_back(make_pair(key, value));
      count++;
   }

   return count;
}


int readmsg(std::istream& sin, map_msgtype& msg)
{
   int     count = 0;
   string  line;
   string  key, value;
   string::size_type  keyend;

   while (mygetline(sin, line)) {
      line = trim(line);
      if (line.size() == 0)  continue;

      // a line starting with '#' is a comment line
      if (*line.begin() == '#')  continue;

      // a line without ':' is a malformed line
      if ((keyend = line.find(':')) == string::npos) continue;

      // extract the key
      key = trim(line.substr(0, keyend));
      if (key.size() == 0)  continue;

      // extract the value
      keyend++;
      value = trim(line.substr(keyend, line.size()-keyend));
      if (value.size() == 0)  continue;

      // insert the (key, value) pair
      if (msg.find(key) == msg.end()) {
	 msg[key] = value;
	 count++;
      }
   }

   return count;
}

int readmsg(std::istream& sin, multimap_msgtype& msg)
{
   int     count = 0;
   string  line;
   string  key, value;
   string::size_type  keyend;

   while (mygetline(sin, line)) {
      line = trim(line);
      if (line.size() == 0)  continue;

      // a line starting with '#' is a comment line
      if (*line.begin() == '#')  continue;

      // a line without ':' is a malformed line
      if ((keyend = line.find(':')) == string::npos) continue;

      // extract the key
      key = trim(line.substr(0, keyend));
      if (key.size() == 0)  continue;

      // extract the value
      keyend++;
      value = trim(line.substr(keyend, line.size()-keyend));
      if (value.size() == 0)  continue;

      // insert the (key, value) pair
      msg.insert(make_pair(key, value));
      count++;
   }

   return count;
}


