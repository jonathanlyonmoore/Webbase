/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 1999-2003 The Board of Trustees of the
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
#ifndef __BIGFILE_H_
#define __BIGFILE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>
#include <ctime>

/** An abstract class to encapsulate the interface to a storage system that can
    handle "very large" files. The methods defined in this class and their semantics
    essentially carry over from the standard POSIX file API. The list of negative
    error codes and their semantics are specific to each implementation of this abstract 
    class.

    Sriram Raghavan <rsram@cs.stanford.edu>
*/

class BigFile {
 public:

  /** Read from the big file
      \param buf: char buffer to read into
      \param size: number of bytes to read. At the end of the call, this will
      contain the actual number of bytes read.
      \return number of bytes read, 0 at end of file, or negative values to indicate 
      various error error conditions
  */
  virtual int read(char *buf, int& size) = 0;

  /** Write to the big file
      \param buf: char buffer containing the bytes to be written
      \param size: maximum number of bytes to write to the file
      \return the actual number of bytes written or negative values to
      indicate various error conditions
  */
  virtual int write(const char* buf, int size) = 0;

  /** Same as POSIX seek function */
  virtual int seek(int64_t offset, int whence) = 0;

  /** \returns Current position of the file pointer */
  virtual int64_t tell() const = 0;

  virtual ~BigFile() {};
};

#endif
