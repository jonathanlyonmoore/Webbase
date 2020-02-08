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
#ifndef CONSTS_H
#define CONSTS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef JUN_ARCHIVER

/** Longest URL the crawler will process. */
/** This value needs to verified with the archiver's interface and limits. */
const int CRAWLER_MAX_URL_SIZE = 10*1024;

/** The maximum number of bytes of a resource that the crawler will save. */
/** This value needs to verified with the archiver's interface and limits. */
const int CRAWLER_MAX_PAGE_SIZE = 1*1024*1024;

#else 

/* These values come from <config.h>, and apply for class repository. */

/** Longest URL the crawler will process. */
const int CRAWLER_MAX_URL_SIZE = MAX_URL_SIZE;

/** The maximum number of bytes of a resource that the crawler will save. */
const int CRAWLER_MAX_PAGE_SIZE = MAX_FILE_SIZE;

#endif

#endif

