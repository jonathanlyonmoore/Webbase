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

#ifndef __PAGECONTAINER_H_
#define __PAGECONTAINER_H_

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string>
#include <cstdio>  // to import SEEK_* constants

/** A PageContainer manages a collection of Web resources ("pages"). The resources are stored 
  * in a BigFile (\see BigFile). Each resource is prefixed with a header containing
  * resource-specific information such as size, URL, HTTP headers, etc. The PageContainer provides
  * methods to read and append pages to the container. It is also possible to seek within a container 
  * using a numeric offset, and to retrieve ("tell") the current offset.
  *
  * Sriram Raghavan <rsram@cs.stanford.edu>
  */
class PageContainer {

 public:

  /** Append a page to the container.
      \param url: the url asssociated with the page
      \param pageBuf: page contents (includes the HTTP header as received by the crawler)
      \param size: the size of the pageBuf buffer in bytes
      \return Non-negative values on success and negative error codes to indicate
              failure.
  */
  virtual int storePage(const std::string url, const char* pageBuf, int size) = 0;

  /** Read a page from the container.
      \param url: at the end of the call, contains the url associated with the page
      \param time: at the end of the call, contains the timestamp indicating when the
                   page was stored in the container
      \param pageBuf: at the end of the call, contains the page contents
      \param size: on entry into the method, must contain the size of the pageBuf
                   buffer; at the end, will contain the number of bytes actually stored
                   in pageBuf.
      \param uncompr: if true, page contents are decompressed before storing in pageBuf
                      if false, pageBuf simply contains byte-by-byte copy of whatever is
                      in the container.
      \return Non-negative values on success and negative error codes to indicate failure
   */
  virtual int readPage(std::string& url, time_t& time, char *pageBuf, int& size, bool uncompr = true) = 0;

  /** Seek into the page container
      \param offset: indicating number of bytes to seek
      \param whence: SEEK_SET, SEEK_CUR, or SEEK_END as with normal POSIX call
      \return Positive value on success and negative on error
  */
  virtual int seek(int64_t offset, int whence) = 0;

  /** \return Current offset or negative value on error
   */
  virtual int64_t tell() const = 0;


  /** Add a name=value style property to a container
   * \return Non-negative value on success and negative error codes to indicate
   *   failure to add property
   */
  virtual int addProperty(std::string propName, std::string propValue) = 0;

  virtual ~PageContainer() {};
};

#endif
