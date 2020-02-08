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
#include <cstring>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "my_utility.h"

using std::string;
using std::vector;


static const string WHITE_SPACES(" \t");


static int split_line(std::string& line, std::string& name, std::string& value)
{
   unsigned  pos;
   unsigned  start, end;

   // remove comment
   pos = line.find('#');
   if (pos != string::npos) {
      if (pos == 0) return -1;
      line.erase(pos);
   }

   // split line into fields
   pos = line.find(':');
   if (pos == string::npos) {
      return -1;
   }
   
   // extract value
   start = line.find_first_not_of(WHITE_SPACES, pos + 1);
   if (start == string::npos) {
      value = "";
   } else {
      end   = line.find_last_not_of(WHITE_SPACES);
      value = line.substr(start, end + 1 - start);
   }
   line.erase(pos);

   // extract name
   start = line.find_first_not_of(WHITE_SPACES);
   if (start == string::npos) return -1;
   end   = line.find_last_not_of(WHITE_SPACES);
   name = line.substr(start, end + 1 - start);

   return 0;
}
   


int parse_robot_file(const std::string& file, const std::string& robot, 
		     std::vector<std::string>& path)
{
   vector<string> lines;
   vector<string> defaultpath, mypath;

   string name, value;
   int    line_num;
   enum apply_dest  { noone, all, me } applyto = noone;
   enum record_type { others, useragent, disallow } prevline = others;

   // split the robot.txt into lines
   split_string(file, "\r\n", lines);

   // extract exclusion list
   for (line_num = 0; line_num < (int)lines.size(); line_num++) {

      // read one line and parse it
      if (split_line(lines[line_num], name, value) < 0)  continue;

      // Is it a "User-Agent" line?
      if (strcasecmp(name.c_str(), "user-agent") == 0) {
	 vector<string> agents;

	 split_string(value, WHITE_SPACES.c_str(), agents);

	 if (prevline != useragent) applyto = noone;
	 for (int i = 0; i < (int)agents.size(); i++) {
	    if (strcasecmp(agents[i].c_str(), robot.c_str()) == 0) {
	       applyto = me;
	       break;
	    }
	    if (agents[i][0] == '*') {
	       applyto = all;
	    }
	 }
	 
	 prevline = useragent;
	 continue;
      }

      // Is it a Disallow line?
      if (strcasecmp(name.c_str(), "disallow") == 0) {
	 switch (applyto) {
	    case me:
	       mypath.push_back(value);
	       break;
	    case all:
	       defaultpath.push_back(value);
	       break;
	    case noone:
	       break;
	 }
	 prevline = disallow;
	 continue;
      }
      
      prevline = others;
   }

   if (mypath.size() > 0) path = mypath;
   else if (defaultpath.size() > 0)  path = defaultpath;
   else  path.erase(path.begin(), path.end());
		       
   return 0;
}

