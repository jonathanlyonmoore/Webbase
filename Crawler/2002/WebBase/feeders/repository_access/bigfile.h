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
#ifndef BIGFILE_H
#define BIGFILE_H

#include <vector>
#include <string>
#include <time.h>
#include <stdio.h>

typedef long long int64;

const unsigned FILE_MAGIC_NUMBER   = 0x1999FEDA;
const int      MAX_FILE_SIZE       = 512 * 1024 * 1024;


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
const int RC_BF_PREMATURE_EOF      = RC_BF_ERROR_START - 14;
const int RC_BF_COMPRESS_ERROR     = RC_BF_ERROR_START - 15;

class bigfile;

class bigfile_manager {
   public:
      bigfile_manager(const vector<string>* dirlist);
      ~bigfile_manager();
      
      int  create(const char* filename);
      int  destroy(const char* filename);
      
      int  open(bigfile& file, const char* filename, char mode);
      int  close(bigfile& file);
      
      int  allocate(string& dir);
      
   private:
      vector<string> dir_list;
};


class bigfile {
   public:
      bigfile();
      ~bigfile();
      int read(char* buf, int& size);
      int write(const char* buf, int size);
      int seek(int64 offset, int whence);
      int64 tell() const;
      int tell(char* file, int& offset) const;

   private:
      int add_chunk();
      int set_current(int file_num);

      bool  bOpen;
      char  mode;
      
      string   main_file;
      string   name;

      vector<string> file;
      vector<long>   file_size;
      int   max_file_size;

      FILE* cur_file;
      int   file_num;

      bigfile_manager *mg;

      friend bigfile_manager;
};

class repository {
public:
  static const int MAX_FILE_SIZE = 1 * 1024 * 1024;
  static const int COMP_BUFFER_SIZE = MAX_FILE_SIZE * 2;

  repository(const char* name,  bool compressed = true, const vector<string>* dirs = NULL, char mode = 'r');
  ~repository();
  int store(const char* name, const char* buf, int size);
  int read(char* name, time_t& time, char* buf, int& size, bool uncompr = true);
  int seek(int64 offset, int whence);
  int64 tell() const;

private:
  bigfile_manager *mg;
  bigfile         *file;
  char  *comp_buf;
  bool compressed;
  int resync(char* name, time_t& time, char* buf, int& size);
};

#endif // BIGFILE_H
