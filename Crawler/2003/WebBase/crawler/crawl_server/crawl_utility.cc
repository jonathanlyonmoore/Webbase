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
#include "crawl_utility.h"

//
// status file implementation - <wlam@cs.stanford.edu> Feb 2001
// all changes to implement this status file are marked
// '// status file - <wlam@cs.stanford.edu> Feb 2001'
//
#include <cstdio>
#include <iostream>
#include <strstream>
#include <sys/time.h>
#include <unistd.h>
#include <cstdlib>
#include <string>

static unsigned long long numpages = 0;
static unsigned long long numbytes = 0;
static struct timeval starttime, lasttime, currenttime;
static int numsites = 0; // Jun 2002

void WriteStatus(void);

void CreateStatusFile(void)
{
   if (gettimeofday(&starttime,NULL) != 0) {
      std::cerr << "Internal error: " << __FILE__ << ':' << __LINE__
                << std::endl;
      exit(1);
   }
   gettimeofday(&lasttime,NULL);

   return;
}

void WriteStatus(void)
{
   gettimeofday(&currenttime,NULL);
   lasttime = currenttime;
   return;
}

#define STATUS_PAGE_SIZE (1024) /* 1K is more than we need */
char status[STATUS_PAGE_SIZE] = { '\0' };
/* The char* returned is static; it is overwritten each call. */
/* The string returned uses \r\n for newlines (appropriate for network). */
char *ReportStatus(void)
{
   WriteStatus();

   // FIXME: should be istringstream as soon as compiler is available
   std::ostrstream stat;
   stat << "Crawler PID " << (int) getpid() << "\r\n"
        << "Start seconds " << starttime.tv_sec << "\r\n"
        << "Status seconds " << currenttime.tv_sec << "\r\n"
        << "Bytes saved " << numbytes << "\r\n"
        << "Pages saved " << numpages << "\r\n"
        << "Sites " << numsites << "\r\n";

   lasttime = currenttime;

   stat << std::ends;
   strcpy(status,stat.str());
   stat.freeze(0);
   return status;
}

void FinishedPage(const int newbytes)
{
   ++numpages;  numbytes += newbytes;
   WriteStatus();

   return;
}

void IncrementSites(void)
{
   ++numsites;  return;
}

void DecrementSites(void)
{
   --numsites;
   if (numsites < 0) {
      std::cerr << "Internal error: " << __FILE__ << ':' << __LINE__ 
                << ": number of currently-crawling sites is negative: "
                << numsites << std::endl;
   }
   return;
}

