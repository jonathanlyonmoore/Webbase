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
/** Utilities for handling HTTP replies. */
/**
   HTTPUtil.cc - Utility functions for handling HTTP server replies.
   Wang Lam <wlam@cs.stanford.edu>
   January 2003

*/
#ifndef HTTPUTIL_H
#define HTTPUTIL_H

#include <string>
#include <cstddef>

/** Retrieves the HTTP header part of an HTTP server response as a string. */
/** This function splits one HTTP response into a header part and a body
    part.  It places the blank-line newline-character(s) that separate the
    header from the body with the header, and returns the header.

    This function is expected to comply with RFC 2616 (HTTP/1.1) and RFC 1945
    (HTTP/1.0 and HTTP/0.9), as specified for tolerant implementations.

    \param httpBuff the HTTP response, as a sequence of bytes (not necessarily
                    NUL-terminated).
    \param bufSize  the number of bytes in the response.
    \return a string containing the HTTP header portion of the response,
            including trailing newlines that separate header from body.
*/
std::string getHTTPHeader(const char* httpBuf, size_t bufSize);

/** Retrieves the HTTP header part of an HTTP server response as a string. */
/** This function splits one HTTP response into a header part and a body
    part.  It places the blank-line newline-character(s) that separate the
    header from the body with the header, and returns the header.  It also
    provides a pointer into httpBuff where the body begins, if there is a 
    body.

    This function is expected to comply with RFC 2616 (HTTP/1.1) and RFC 1945
    (HTTP/1.0 and HTTP/0.9), as specified for tolerant implementations.

    \param httpBuff the HTTP response, as a sequence of bytes (not necessarily
                    NUL-terminated).
    \param bufSize  the number of bytes in the response.
    \param data     address of a const char *, which will be filled with
                    a pointer to the first byte of the HTTP-response
                    body inside httpBuff.  If there is no body, data
                    holds (httpBuf + bufSize), which points past httpBuff[].
                    data is never filled with NULL.
    \return a string containing the HTTP header portion of the response,
            including trailing newlines that separate header from body.
*/
std::string getHTTPHeader(const char* httpBuf, size_t bufSize,
                          const char** data);

#endif

