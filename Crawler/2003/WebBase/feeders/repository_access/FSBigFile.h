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
#ifndef __FSBIGFILE_H_
#define __FSBIGFILE_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>
#include <ctime>
#include "BigFile.h"

/** List of error codes that FSBigFile can return */
const int RC_BF_ERROR_START        = -1000;
const int RC_BF_EOF                = RC_BF_ERROR_START - 0;
const int RC_BF_DISK_FULL          = RC_BF_ERROR_START - 1;

const int RC_BF_CANNOT_CREATE_FILE = RC_BF_ERROR_START - 2;
const int RC_BF_CANNOT_OPEN_FILE   = RC_BF_ERROR_START - 3;
const int RC_BF_FILE_ACCESS_ERROR  = RC_BF_ERROR_START - 4;
const int RC_BF_READ_ERROR         = RC_BF_ERROR_START - 5;
const int RC_BF_WRITE_ERROR        = RC_BF_ERROR_START - 6;

const int RC_BF_INVALID_FILE       = RC_BF_ERROR_START - 7;
const int RC_BF_FILE_OPEN          = RC_BF_ERROR_START - 8;
const int RC_BF_FILE_CLOSED        = RC_BF_ERROR_START - 9;
const int RC_BF_READ_ONLY          = RC_BF_ERROR_START - 10;
const int RC_BF_WRITE_ONLY         = RC_BF_ERROR_START - 11;

const int RC_BF_BUFFER_TOO_LONG    = RC_BF_ERROR_START - 12;
const int RC_BF_INVALID_PARAMETER  = RC_BF_ERROR_START - 13;

class chunk_manager;

/** Implements the BigFile interface over a file system that cannot natively
    support files > 2GB in size.
*/
class FSBigFile: public BigFile {

 public:

  /** Constructor:
      \param chunkListFile: File containing the list of chunks making up the bigfile 
      \param dirlist: a list of directories in which the chunks will be stored
      \param mode: mode (r, w, or a) in which to open the bigfile
  */
  FSBigFile(const char *chunkListFile, const std::vector<std::string>* dirlist = NULL, char mode = 'r');

  /** Destructor */
  virtual ~FSBigFile();

  /** The methods corresponding to the BigFile interface. 
      \see BigFile 
  */
  int read(char* buf, int& size);
  int write(const char* buf, int size);
  int seek(int64_t offset, int whence);
  int64_t tell() const;

 private:
  int tell(char* file, int& offset) const;
  int add_chunk();
  int set_current(int file_num);
  
  bool bOpen;
  char mode;
  std::string main_file;
  std::string name;
  std::vector<std::string> file;
  std::vector<long> file_size;
  int max_file_size;
  FILE* cur_file;
  int file_num;
  
  chunk_manager *mg;
  friend class chunk_manager;
};


/** Friend class used by FSBigFile for disk space management, to allocate space in the 
    appropriate directories, and to manage the list of chunks that make up a big file.
*/
class chunk_manager {
 public:
  chunk_manager(const std::vector<std::string>* dirlist);
  ~chunk_manager();
  
  int create(const char* filename);
  int destroy(const char* filename);
  int open(FSBigFile& file, const char* filename, char mode);
  int close(FSBigFile& file);
  int allocate(std::string& dir);
  
 private:
  std::vector<std::string> dir_list;
};

#endif
