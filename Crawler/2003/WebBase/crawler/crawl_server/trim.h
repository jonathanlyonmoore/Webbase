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
#ifndef TRIM_H
#define TRIM_H

#include <string>
#include <cctype>

template <class CharT, class Traits, class Alloc>
std::basic_string<CharT, Traits, Alloc>
trim(const std::basic_string<CharT, Traits, Alloc>& line)
{
   typedef typename std::basic_string<CharT, Traits, Alloc>::size_type size_type;
   size_type  begin = 0, end = line.size();
  
   while (begin != end && isspace(line[begin])) begin++;
   while (begin != end && isspace(line[end-1])) end--;

   return line.substr(begin, end-begin);
}

#endif /* TRIM_H */
