/*
   The Stanford WebBase Project <webbase@db.stanford.edu>
   Copyright (C) 2003 The Board of Trustees of
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

#ifndef WBHEADER_H
#define WBHEADER_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "BigFile.h"   
#include "repository.h"
#include "Crc32.h"
#include <string>
#include <netinet/in.h>


/**
 *                  WebBase Header (WBHeader class)
 *
 * Implements the WebBase header format. A header conforming to this format 
 * is prefixed to every page in the repository.
 *
 * In the following, | denotes concatenation
 * 
 * Header format:   Header = FixedPart | VariableLengthPart | CheckSum
 * 
 *    FixedPart = 7 entries each sizeof(unsigned) bytes long [4 bytes usually]
 *                (1)Magic-code  (2)Header version#  (3)Timestamp 
 *                (4)Uncompressed page size (5)Compressed page size
 *                (6)Url length (7)Length of HTTP headers
 *
 *    VariableLengthPart = URL string | HTTP Headers
 *
 *    Checksum = CRC32 over (FixedPart | VariableLenghPart)
 *
 * Note: 1. On disk, all 7 entries are stored in Network byte order (High-endian)
 *       2. URL and HTTP header strings are not null-terminated. Must use length info 
 *          from FixedPart to read and process.
 *       3. When computing CheckSum, all entries in FixedPart are in network byte order.
 * 
 *  
 * Modified by Sriram Raghavan <rsram@cs.stanford.edu>  [04/2003]
 *        from Gary Wesley's (gary@db) implementation   [03/2003]
 * 
 *
 */

const unsigned FILE_MAGIC_CODE   = 0xFEDB2003; // unique header type identifier
                                               // dec of  FEDB2003 = 4275773443
const unsigned HDR_VERSION         = 2;
const int      HDR_PIECE_SIZE      = sizeof(unsigned); 
const int      NUM_HDR_PIECES      = 7;                 // fixed portion
const int      HDR_FIXED_PART_SIZE = HDR_PIECE_SIZE * NUM_HDR_PIECES;

// what position in the HDR array
const int MAGICCODE_INDEX          = 0;  
const int VERSION_INDEX            = 1;  
const int TIMESTAMP_INDEX          = 2;  
const int UNCOMPRESSED_SIZE_INDEX  = 3;  
const int COMPRESSED_SIZE_INDEX    = 4; 
const int URL_LENGTH_INDEX         = 5;   
const int HTTP_HEADER_LENGTH_INDEX = 6;  


class WBHeader {
 public:

    WBHeader(BigFile *); 
    WBHeader(std::string URL, const char * pageBuf, int pageBufSize); 

    ~WBHeader() {
      if (headerString != NULL) delete headerString;
    }

    unsigned getMagicCode()       { return headerFixedPartLBO[ MAGICCODE_INDEX ] ;}
    unsigned getVersion ()        { return headerFixedPartLBO[ VERSION_INDEX ] ;}
    unsigned getURLlength()       { return headerFixedPartLBO[URL_LENGTH_INDEX  ] ;}
    time_t   getTimestamp()       { return headerFixedPartLBO[TIMESTAMP_INDEX  ] ;}
    unsigned getUncompressedSize(){ return headerFixedPartLBO[UNCOMPRESSED_SIZE_INDEX  ] ;}
    unsigned getCompressedSize()  { return headerFixedPartLBO[COMPRESSED_SIZE_INDEX  ] ;}
    unsigned getHTTPheaderLength(){ return headerFixedPartLBO[HTTP_HEADER_LENGTH_INDEX  ] ;}
    unsigned getChecksum()        { return checkSum ;}
    std::string getURLstr()            { return URLstr  ;}
    std::string getHTTPheader()        { return HttpHeader  ;}
    int getSize()    { return HDR_FIXED_PART_SIZE + URLstr.length() + HttpHeader.length() + sizeof(checkSum); }


    int setUncompressedSize( unsigned i){
      headerFixedPartLBO[ UNCOMPRESSED_SIZE_INDEX]= i;
      headerFixedPartNBO[ UNCOMPRESSED_SIZE_INDEX]= htonl( i );
    }
    int setCompressedSize(   unsigned i){
      headerFixedPartLBO[ COMPRESSED_SIZE_INDEX  ]= i;
      headerFixedPartNBO[ COMPRESSED_SIZE_INDEX]= htonl( i );
    }
    int setURLstr( std::string   s){ 
      URLstr              = s; 
      headerFixedPartLBO[ URL_LENGTH_INDEX ] = URLstr.length();
      headerFixedPartNBO[ URL_LENGTH_INDEX ] = htonl( URLstr.length() );
    }
    int setHTTPheader( std::string s){ 
      HttpHeader            = s; 
      headerFixedPartLBO[ HTTP_HEADER_LENGTH_INDEX] = HttpHeader.length();
      headerFixedPartNBO[ HTTP_HEADER_LENGTH_INDEX] = htonl(HttpHeader.length() );
    }
    
    // Is this the Magic Code? Parameter "bytes" must be in network order as read from disk
    static bool isMagicCode(char bytes[HDR_PIECE_SIZE]){
      unsigned temp = *((unsigned long *)bytes);
      return FILE_MAGIC_CODE == ntohl(temp);
    }

    bool isValidHeader();
    const char *castHeader();
    
 private:
    
    bool makeHeaderString();
    bool isFixedPartSane();
    
    unsigned headerFixedPartLBO[NUM_HDR_PIECES]; // fixed part in Local Byte Order
    unsigned headerFixedPartNBO[NUM_HDR_PIECES]; // fixed part in Network Byte Order
    std::string URLstr;
    std::string HttpHeader;
    unsigned checkSum;      // computed checksum on fixed + URLstr +  HttpHeader
    unsigned onDiskcheckSum; // checksum as read from disk

    char fixedPartBuf[HDR_FIXED_PART_SIZE];
    char *headerString;
    bool initialized;
};

#endif // WBHEADER_H
