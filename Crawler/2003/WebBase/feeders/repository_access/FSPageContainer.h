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

#ifndef __FSPAGECONTAINER_H_
#define __FSPAGECONTAINER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "FSBigFile.h"
#include "PageContainer.h"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

/** Implements the PageContainer interface on a regular file system using an FSBigFile.
  *
  *  Sriram Raghavan <rsram@cs.stanford.edu>
  */
class FSPageContainer: public PageContainer {

 public:

  static const int TEMPORARY_BUFFER_SIZE = MAX_FILE_SIZE;
  static const int RESYNC_SEARCH_WINDOW = 10 * 1024;

  static const int SUCCESS = 0;
  static const int COMPRESS_ERROR = -1;
  static const int PAGE_TOO_LARGE = -2;
  static const int COMPRESSION_MISMATCH = -3;
  static const int BIGFILE_ERROR = -4;
  static const int FAILURE_TO_LOAD_RESYNC_BUFFER = -5;
  static const int WRITE_TO_PROPFILE_FAILED = -6;

  /** Constructor to initialize a file system page container
     \param containerName: name of the container. This parameter is actually a path to 
                           the chunklistfile that lists all the chunks that make up the container.
                           This parameters is simply passed along to FSBigFile.
     \param compressed: if true, pages will be compressed before storage.
     \param dirlist: directories in which the container will be placed. Directly passed
                     along to FSBigFile.
     \param mode: mode in which to open the container (r or w)
  */
  FSPageContainer(const std::string containerName, bool compressed = true, 
                  const std::vector<std::string>* dirlist = NULL, char mode='r');

  /** Destructor */
  virtual ~FSPageContainer();

  /** Methods corresponding to the PageContainer interface. 
      \see PageContainer
  */
  int storePage(const std::string url, const char* pageBuf, int size);
  int readPage(std::string& url, time_t& time, char *pageBuf, int& size, bool uncompr = true);
  int seek(int64_t offset, int whence);
  int64_t tell() const;
  int addProperty(std::string propName, std::string propValue);

private:
  BigFile *bigfile;           // the BigFile that is used to store the container
  std::string propFile;       // full path to file in which to store containe properties
  static  char *temporary_buf;        // temporary copression/decompression buffer
  static  char *resync_buf;           // buffer used by resync to hold current search window
  bool compressed;            // set to true if the pages in the container are stored compressed
  ostringstream msgStream;    // string stream to log error/status messages

  inline void logMessage();
  int resync(int64_t startPos, std::string& url, time_t& time, char* pageBuf, int& size, bool uncompr);
};

#endif
