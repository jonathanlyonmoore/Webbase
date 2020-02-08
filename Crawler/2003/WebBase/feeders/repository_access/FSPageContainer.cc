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

/** Implementations for the methods in the FSPageContainer class
  *
  * Sriram Raghavan <rsram@cs.stanford.edu>
  */

#include "FSPageContainer.h"
#include "WBHeader.h"
#include <zlib.h>

char*  FSPageContainer::temporary_buf = new char[TEMPORARY_BUFFER_SIZE];
char*  FSPageContainer::resync_buf = new char[RESYNC_SEARCH_WINDOW];

FSPageContainer::FSPageContainer(const std::string containerName, bool compressed, 
                                 const std::vector<std::string>* dirlist, char mode)
{
  this->compressed = compressed;
  bigfile = new FSBigFile(containerName.c_str(), dirlist, mode);
  if (mode == 'w') {
    propFile = (*dirlist)[0] + "/containerProperties";
    ofstream containerPropFile(propFile.c_str());
    if (containerPropFile.is_open()) {
      containerPropFile << "#Properties File for Container " << containerName << endl;
      containerPropFile.close();
    }
    else
      cerr << "FSPageContainer: Unable to create properties file " << propFile 
           << ". Continuing anyway.. " << endl;
  }
}


FSPageContainer::~FSPageContainer()
{
  delete bigfile;
}

void FSPageContainer::logMessage()
{
  cerr << "FSPageContainer: " << msgStream.str() << endl;
  msgStream.str("");
}


int FSPageContainer::seek(int64_t offset, int whence)
{
  int rc = bigfile->seek(offset, whence);
  if (rc < 0) {
    msgStream << "Seek error. Bigfile error code " << rc;  logMessage();
    return BIGFILE_ERROR;
  }
  return SUCCESS;
}


int64_t FSPageContainer::tell() const
{
  int rc = bigfile->tell();
  if (rc < 0) {
    cerr << "FSPageContainer: Tell error. Bigfile error code " << rc << endl;
    return BIGFILE_ERROR;
  }
  return rc;
}


int FSPageContainer::addProperty(std::string propName, std::string propValue)
{
  int returnCode = WRITE_TO_PROPFILE_FAILED;
  ofstream containerPropFile(propFile.c_str(), ios::app);

  if (containerPropFile.is_open()) {
    containerPropFile << propName << "=" << propValue << endl;
    if (containerPropFile.good()) returnCode = SUCCESS;
    containerPropFile.close();
  }
  return returnCode;
}


int FSPageContainer::storePage(const std::string url, const char* pageBuf, int size)
{
  // Create WebBase page header
  WBHeader wbheader(url, pageBuf, size);
  wbheader.setUncompressedSize((unsigned) size);
  int wbheaderSize = wbheader.getSize();

  msgStream << "Creating WBHeader of size = " << wbheaderSize; logMessage();

  int writeBufSize;
  const char *writeBuffer;

  // compress the page and add to write buffer; 
  if (compressed) {

    // size of compressed page must be at most (MAX_FILE_SIZE - size of its WBHeader)
    // by setting destSize to this value, we force zlib to report an error if
    // the compressed page is larger
    uLongf destSize = TEMPORARY_BUFFER_SIZE - wbheaderSize;

    int rc = compress((Bytef *)temporary_buf, &destSize, (Byte*)pageBuf, (uLong)size);
    if (rc != Z_OK) {
      msgStream << "Error compressing " <<  url << ". Zlib error code " << rc; logMessage();
      return COMPRESS_ERROR;
    }
    wbheader.setCompressedSize((unsigned) destSize);
    writeBuffer = temporary_buf;
    writeBufSize = destSize;

    msgStream << "Page compressed successfully [" << destSize << "," << size << "]"; logMessage();
  }


  // no compression: writebuffer directly points to incoming page buffer
  else {

    // first check if page + header will fit into MAX_FILE_SIZE
    if (size + wbheaderSize > MAX_FILE_SIZE) {
      msgStream << "Incoming resource + header too large for url "  << url; logMessage();
      return PAGE_TOO_LARGE;
    }
    wbheader.setCompressedSize((unsigned) size);
    writeBuffer = pageBuf;
    writeBufSize = size;
  }

  // Write the header to disk
  int bf_returnCode = bigfile->write(wbheader.castHeader(), wbheaderSize);
  if (bf_returnCode < 0) {
    msgStream << "Error writing header for " << url 
              << " to disk. Bigfile error code " << bf_returnCode;
    logMessage();
    return BIGFILE_ERROR;
  }
  msgStream << "Webbase header successfully written to disk "; logMessage();

  // Write the (compressed) page to disk
  bf_returnCode = bigfile->write(writeBuffer, writeBufSize);
  if (bf_returnCode < 0) {
    msgStream << "Error writing " << url << " to disk. Bigfile error code " << bf_returnCode;
    logMessage();
    return BIGFILE_ERROR;
  }

  // Page successfully stored
  msgStream << "Page write succeeded"; logMessage();
  return SUCCESS;
}


int FSPageContainer::readPage(std::string& url, time_t& time, char *pageBuf, int& size, bool uncompr)
{

  if ( !compressed && !uncompr) {
    msgStream << "Compressed pages requested when pages not stored compressed"; logMessage();
    return COMPRESSION_MISMATCH;
  }

  // record current position
  int64_t pos = bigfile->tell();
  if (pos < 0)  {
    msgStream << "Could not record start position in readPage. Bigfile error code " << pos;
    logMessage();
    return BIGFILE_ERROR;
  }

  // Read from bigfile and create the header structure
  WBHeader wbheader(bigfile);
  if (! wbheader.isValidHeader()) {
    msgStream << "Invalid header detected";
    logMessage();
    return resync(pos + 1, url, time, pageBuf, size, uncompr);
  }

  // Valid header
  else {
    // assign timestamp and URL to out-parameter
    time = wbheader.getTimestamp();
    url = wbheader.getURLstr();

    int bytesRead;
    int processingStatus;
    
    // Read and uncompress
    if (compressed && uncompr) {
      bytesRead = wbheader.getCompressedSize();
      processingStatus = bigfile->read(temporary_buf, bytesRead);
      if (processingStatus < 0 || bytesRead < wbheader.getCompressedSize()) {
        msgStream << "Error reading compressed page from disk for url " << url << endl;
        msgStream << "Bigfile return code " << processingStatus;
        logMessage();
        return BIGFILE_ERROR;
      }

      uLongf destSize = size;
      processingStatus = uncompress((Bytef *) pageBuf, &destSize,
                                    (Bytef *) temporary_buf, (uLong) bytesRead);
      if (processingStatus != Z_OK || (unsigned)destSize != wbheader.getUncompressedSize()) {
        msgStream << "Decompression error when retrieving url " << url << endl;
        msgStream << "Zlib return code " << processingStatus;
        logMessage();
        return resync(pos + 1, url, time, pageBuf, size, uncompr);        
      }
      else size = destSize;
    }

    // No uncompression. Simply copy over to caller's buffer
    else {

      bytesRead = wbheader.getCompressedSize();
      processingStatus = bigfile->read(pageBuf, bytesRead);
      if (processingStatus < 0 || bytesRead < wbheader.getCompressedSize()) {
        msgStream << "Error reading uncompressed page from disk for url " <<  url; logMessage();
	msgStream << "processingStatus: " << processingStatus; logMessage();
	msgStream << "bytesRead: " << bytesRead << "\t" << "getCompressedSize(): " << wbheader.getCompressedSize(); logMessage();
	msgStream << "bigfile->tell(): " << bigfile->tell(); logMessage();
        return BIGFILE_ERROR;

      /* BUG!!
      bytesRead = wbheader.getUncompressedSize();
      processingStatus = bigfile->read(pageBuf, bytesRead);
      if (processingStatus < 0 || bytesRead < wbheader.getUncompressedSize()) {
        msgStream << "Error reading uncompressed page from disk for url " <<  url; logMessage();
        return BIGFILE_ERROR;
      */

      }
      else size = bytesRead;
    }
  }
  return SUCCESS;
}


int FSPageContainer::resync(int64_t startPos, std::string& url, time_t& time, char* pageBuf, 
                            int& size, bool uncompr)
{
  msgStream << "Attempting resync at position " << startPos; logMessage();

  // Seek to specified start position to being resync operation
  int rc = bigfile->seek(startPos, SEEK_SET);
  if (rc < 0) {
    msgStream << "Error seeking to resync start position. Bigfile error code " << rc;
    logMessage();
    return BIGFILE_ERROR;
  }

  while (1) {

    // Store initial position of resync window
    int64_t resyncBegin = bigfile->tell();
    if (resyncBegin < 0) {
      msgStream << "Error in tell. Failed to get initial position of resync window." << endl;
      msgStream << "Bigfile error code " << resyncBegin;
      logMessage();
      return BIGFILE_ERROR;
    }

    // Load the resync buffer
    int resyncSize = RESYNC_SEARCH_WINDOW;
    if ((rc = bigfile->read(resync_buf, resyncSize)) < 0) {
      msgStream << "Error loading resync buffer. Bigfile error code " << rc;
      logMessage();
      return FAILURE_TO_LOAD_RESYNC_BUFFER;
    }
    //  if (resyncSize < RESYNC_SEARCH_WINDOW) return RC_BF_PREMATURE_EOF;


    msgStream << "Searching for magic code in " << RESYNC_SEARCH_WINDOW << " bytes "
              << "beginning at " << resyncBegin;
    logMessage();

    // Search for the magic code. If found, seek to that point and try reading again
    for (int i = 0; i <= resyncSize - 4; i++) {
      if (WBHeader::isMagicCode(resync_buf + i)) {
        int syncPoint = resyncBegin + i;
        if ((rc = bigfile->seek(syncPoint, SEEK_SET)) < 0) {
          msgStream << "Error seeking to possible sync point " << syncPoint 
                    << ". Bigfile error code " << rc;
          logMessage();
          return BIGFILE_ERROR;
        }
        return readPage(url, time, pageBuf, size, uncompr);
      }
    }

    // Magic code not found in current resync buffer. Load next set of
    // bytes into resync buffer and start over.
  }
}
