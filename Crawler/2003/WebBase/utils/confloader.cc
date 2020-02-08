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
/*                         WebBase project
 *                         ---------------
 * Method definitions for the ConfLoader class
 * 
 * Author: Sriram Raghavan - rsram@cs.stanford.edu
 */

#include <iostream>
#include <string>
#include <map>
#include <fstream>
#include "confloader.h"

using namespace std;


// Empty default constructor

ConfLoader::ConfLoader() { }


// Reads the configuration file and loads the (parameter,value) pairs
// into memory. Stores it in a "map" data structure.
// 
// confFile : full pathname of the configuration file
// delimiter : the character that is used to delimit the parameter from
//             the value in a given line of the configuration file
// 
// Returns true on success and false if it encounters a problem

bool ConfLoader::loadValues(const char *confFile, char delimiter)
{
  ifstream cfile(confFile);      // get an input stream to config file
  string line;                  

  if (! cfile.is_open()) {
    cerr << "Error opening config file : " << confFile << endl;
    return(false);
  }
  while (!cfile.eof()) {
    getline(cfile, line);                 // read a line at a time
    if (line.find('#') == 0) continue;    // ignore if comment line    

    // wlam: Implement an "include configfile" command.
    if (line.substr(0,sizeof("include ")-1)=="include ") {
       // cerr << "Including file " << line << endl;
       // FIXME:
       // This code should check for circular inclusion so that
       // this code can't blow the stack.  Oh, well.  :P
       loadValues(line.substr(sizeof("include ")-1,
                              line.length()-sizeof("include ")+1).c_str(),
                  delimiter);
    }
    // wlam - end edit 16 Aug 2001

    // I am sure there is a better way to strip out white space from a 
    // string but I guess I am just too lazy to find out. So here goes ..

    string stripped_line;
    string::iterator iter = line.begin();
    while (iter != line.end()) {
      if (! isspace(*iter)) stripped_line += *iter;
      iter++;
    }

    int pos = stripped_line.find(delimiter);       
    if (pos == stripped_line.npos) continue;     // ignore if no delimiter found

    string name = stripped_line.substr(0, pos);    // split into param and value
    string value = stripped_line.substr(pos+1, stripped_line.length()-pos);
    cerr << "Loading : Parameter=" << name << " Value=" << value << endl;
    confMap[name] = value;                 // add an entry to the map
  }
  cfile.close();
  return(true);
}

// Returns the value associated with a certain configuration parameter.
// propertyName : the name of the parameter for which the value is needed
// 
// Returns "" if the parameter has not been loaded into memory. Otherwise
//         returns the value associated with that parameter

const char *ConfLoader::getValue(const char *propertyName)
{
  if (confMap.find(propertyName) == confMap.end()) return("");
  return( (confMap[propertyName]).c_str());
}
