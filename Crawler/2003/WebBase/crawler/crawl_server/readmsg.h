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
#ifndef READMSG_H
#define READMSG_H

#include <iostream>
#include <map>
#include <vector>
#include <string>

typedef std::vector<std::pair<std::string, std::string> > vec_msgtype;
typedef std::map<std::string, std::string>                map_msgtype;
typedef std::multimap<std::string, std::string>           multimap_msgtype;

int readmsg(std::istream& sin, vec_msgtype& msg);
int readmsg(std::istream& sin, map_msgtype& msg);
int readmsg(std::istream& sin, multimap_msgtype& msg);
inline int writemsg(std::ostream& sout, const std::string& key,
                    const std::string& value) {
   sout << key << ": " << value << std::endl;
   return (sout) ? 0 : -1;
}

#endif /* READMSG_H */
