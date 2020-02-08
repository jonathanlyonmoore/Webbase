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
#ifndef MYGETLINE_H
#define MYGETLINE_H

#include <istream.h>
#include <string>

template <class CharT, class Traits, class Alloc>
istream& mygetline(istream& in, basic_string<CharT, Traits, Alloc>& line)
{
   getline(in, line);
   if (line.size() > 0 && *(line.end()-1) == '\r') {
      line.erase(line.end()-1);
   }
   return in;
}

#endif /* MYGETLINE_H */
