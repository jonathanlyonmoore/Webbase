/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 2003 The Board of Trustees of the
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


/**                 WebBase Header Implementation
 *
 *
 *    Modified by Sriram Raghavan <rsram@cs.stanford.edu>     [04/2003]
 *    from Gary Wesley's (gary@db.stanford.edu)implementation [3/2003]
 */

      
#include "WBHeader.h"
#include <iostream.h>
#include <strstream.h>
#include <time.h>
#include <math.h>
#include "Crc32.h"
#include "HTTPUtil.h"

/** Check if fixed part of the header at least looks sane, based on
 * equality and range checks.
 */
bool WBHeader::isFixedPartSane()
{
  if ( (headerFixedPartLBO[MAGICCODE_INDEX] != FILE_MAGIC_CODE) ||
       (headerFixedPartLBO[VERSION_INDEX] != HDR_VERSION) ||
       (headerFixedPartLBO[COMPRESSED_SIZE_INDEX] <= 0) ||
       (headerFixedPartLBO[COMPRESSED_SIZE_INDEX] > MAX_FILE_SIZE) ||
       (headerFixedPartLBO[UNCOMPRESSED_SIZE_INDEX] <= 0) ||
       (headerFixedPartLBO[UNCOMPRESSED_SIZE_INDEX] > MAX_FILE_SIZE) ||
       (headerFixedPartLBO[URL_LENGTH_INDEX] <= 0) ||
       (headerFixedPartLBO[URL_LENGTH_INDEX] > MAX_URL_SIZE) ) {
    cerr << "WBHeader: Fixed part of header fails one or more sanity checks" << endl;
    return false;
  }
  return true;
}


/** Check if the header read from disk is valid
 * 
 *  Checks include: checksum compute and match with value read from disk
 *                  magic code and version number values must be as expected
 *                  URL and page length information must be within expected limits                    
 */
bool WBHeader::isValidHeader()
{
  if (! initialized) {
    cerr << "WBHeader: Header structure not initialized." << endl;
    return false;
  }

  if (checkSum != onDiskcheckSum) {
    cerr << "WBHeader: Checksum mismatch" << endl;
    return false;
  }

  if (! isFixedPartSane()) return false;

  return true;
}


/**
 * Writes the entire header out as a string, ready  for transfer to disk.
 */
bool WBHeader::makeHeaderString()
{
  headerString = new char[getSize() + 1];
  if (headerString == NULL) {
    cerr << "WBHeader: Unable to allocate memory to store WBHeader as a string" << endl;
    return false;
  }

  int currentLen = HDR_FIXED_PART_SIZE;
  memcpy(headerString, headerFixedPartNBO, currentLen); // note: CRC computed on NBO version of fixed part

  memcpy(headerString + currentLen, URLstr.c_str(), URLstr.length());
  currentLen += URLstr.length();

  memcpy(headerString + currentLen, HttpHeader.c_str(), HttpHeader.length());
  currentLen += HttpHeader.length();

  static Crc32 crc;
  crc.reset();
  crc.Compute(headerString, currentLen);
  checkSum = (unsigned long) crc;
  unsigned NBOchecksum = htonl(checkSum);

  memcpy(headerString + currentLen, (char *)&NBOchecksum, sizeof(NBOchecksum));
  headerString[currentLen + sizeof(checkSum)] = '\0';

  return true;
}


/** Constructor for initializing a header structure from bytes on disk
 *
 * Note: At the end of the constructor, if the header is initialized properly,
 * the file pointer associated with the bigfile "bf" will be pointing past
 * the WebBase header at the beginning of the actual (compressed) page.
 */
WBHeader::WBHeader(BigFile *bf)
{
  initialized = false;
  headerString = NULL;
  
  // Read in fixed part
  int fixedPartBufSize = HDR_FIXED_PART_SIZE;
  int returnCode = bf->read(fixedPartBuf, fixedPartBufSize);
  if ( (returnCode < 0) || (fixedPartBufSize != HDR_FIXED_PART_SIZE) ) {
    cerr << "WBHeader: Error reading from bigfile; error code = " << returnCode << endl;
    return;
  }

  // Assign to various components of the fixed part, in network and local host order
  for (int i = 0; i < NUM_HDR_PIECES ; i++){
    headerFixedPartNBO[i] = *((unsigned *)(fixedPartBuf + i * HDR_PIECE_SIZE));
    headerFixedPartLBO[i] = ntohl(headerFixedPartNBO[i]);
  }

  // Perform sanity check on the fixed part
  if (! isFixedPartSane()) return;

  // allocate memory to read in url, http headers, and on-disk checksum value
  int urlLength = headerFixedPartLBO[URL_LENGTH_INDEX];
  int httpHeaderLength = headerFixedPartLBO[HTTP_HEADER_LENGTH_INDEX];
  int bytesToRead =  urlLength + httpHeaderLength + sizeof(checkSum);
  char *readBuffer = new char[bytesToRead];
  if (readBuffer == NULL) {
    cerr << "WBHeader: Unable to allocate memory for reading in URL and HTTP headers " << endl;
    return;
  }

  // read in url, http headers, and on-disk checksum value
  int bytesRead = bytesToRead;
  returnCode = bf->read(readBuffer, bytesRead);
  if ((returnCode < 0) || (bytesRead != bytesToRead)) {
    cerr << "WBHeader: Error reading url, http headers, and checksum. Bigfile error code = " << returnCode << endl;
    delete [] readBuffer;
    return;
  }

  string urlstring(readBuffer, urlLength);
  string httpstring(readBuffer + urlLength, httpHeaderLength);
  URLstr = urlstring;
  HttpHeader = httpstring;

  unsigned NBOonDiskcheckSum;
  memcpy((char *)&NBOonDiskcheckSum, readBuffer + urlLength + httpHeaderLength, sizeof(NBOonDiskcheckSum));
  onDiskcheckSum = ntohl(NBOonDiskcheckSum);

  initialized = makeHeaderString();
  delete [] readBuffer;
}  


/** Constructor to build a WBHeader structure, given a URL and page contents
 *
 *  This constructor will set all entries of the header except the 5th
 *  entry in the fixed part (COMPRESSED SIZE of the PAGE) - that value
 *  will have to be set separately using the setCompressedSize() method.
 */
WBHeader::WBHeader(std::string URL, const char * pageBuf, int pageBufSize)
{
  headerFixedPartLBO[MAGICCODE_INDEX]  = FILE_MAGIC_CODE;
  headerFixedPartLBO[VERSION_INDEX]    = HDR_VERSION ;
  headerFixedPartLBO[TIMESTAMP_INDEX ] = (unsigned) time(NULL);
  headerFixedPartLBO[UNCOMPRESSED_SIZE_INDEX] = pageBufSize;
  headerFixedPartLBO[COMPRESSED_SIZE_INDEX] = 0;   // will be set later by call to the "set" method

  setURLstr(URL);
  setHTTPheader(getHTTPHeader(pageBuf, pageBufSize)); 

  for (int i = 0; i < NUM_HDR_PIECES; i ++)
    headerFixedPartNBO[i] = htonl(headerFixedPartLBO[i]);

  initialized = true;
}


/** Serialize the header and produce a string that can be written to disk
 */
const char * WBHeader::castHeader() 
{
  if (! initialized) return NULL;
  makeHeaderString();
  return headerString;
}
