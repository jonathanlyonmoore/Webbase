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
/*
 * blinkgen.h
 * 
 * Taher H. Haveliwala (taherh@cs.stanford.edu)
 */

#ifndef __blinkgen_h__
#define __blinkgen_h__

#include <iostream.h>
#include <fstream.h>

#define MAX_LINE 255

class BLinkGen {
private:
  ofstream files[64];

  ofstream& getFileFor(unsigned long long id);

  bool createFileTable(const char* linksdir, const char* baseFilename);

  bool append;

public:
  BLinkGen(const char* linksdir, const char* baseFilename, bool append0);

  bool flipLink(unsigned long long sourceId, 
		 vector<unsigned long long int>& destIds) ;

};


#endif // __blinkgen_h__
