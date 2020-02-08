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
 * Interface definition for a class that reads the configuration file 
 * and creates an associative map in memory. Provides a method for returning
 * the value assigned to a certain parameter.
 * 
 * Author: Sriram Raghavan - rsram@cs.stanford.edu
 */

#ifndef __confloader_h_
#define __confloader_h_

#include <string>
#include <map>

class ConfLoader {
private :
  std::map<std::string,std::string> confMap;        // map to store the (parameter,value) pairs
public:
  ConfLoader();                      
  bool loadValues(const char *confFile, char delimiter);
  const char *getValue(const char *propertyName);
};

#endif
