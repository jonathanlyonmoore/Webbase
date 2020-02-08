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
/*                 Header file for the normalize function
 *                 --------------------------------------
 * The normalize function (defined in normalize.cc) normalizes URLs in a consistent manner. The
 * actual normalization process is described in the documentation associated with the function
 * definition.
 * Author :  Sriram Ragahvan - rsram@cs.stanford.edu
 */

#ifndef __NORMALIZE_H_
#define __NORMALIZE_H_

#include <string.h>
#include <ctype.h>

#define MAX_LINE_LENGTH (1024+1)   // max. length of URL (including \0) handled by this function.

/* Normalizes the URL pointed to by "url". Does the following 
 *    - removes a leading http:// if one exists,
 *    - removes multiple trailing slashes if any
 *    - converts the initial server portion of the URL to lower case
 *    - if a :80 port specification is encountered, that is stripped away
 * Returns 1 and normalizes properly if the string begins with "http://" or
 * "HTTP://" (not even white space is allowed in the beginning of the url). 
 * If these conditions are not satisfied, returns 0 and does not normalize.
 * NOTE : This function modifies the contents of the buffer pointed to by "url". However it is
 *  guaranteed that there will be no buffer overflow since the function only removes stuff and
 *  does not add anything to the buffer contents.
 */

/* This makes normalize() available to C.  (W3C's libwww is.)
   Wang Lam <wlam@cs.stanford.edu>, 5 Mar 2001

   normalize.cc (described above) is being phased out in favor of normalize.c,
   which uses libwww.
   The normalization is different, but the function normalize() is used
   in the same way.
 */
#ifdef __cplusplus
extern "C" {
#endif

int normalize(char *url);

#ifdef __cplusplus
}
#endif

#endif
