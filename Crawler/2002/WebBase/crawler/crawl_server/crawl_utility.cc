#include "crawl_utility.h"

//
// status file implementation - <wlam@cs.stanford.edu> Feb 2001
// all changes to implement this status file are marked
// '// status file - <wlam@cs.stanford.edu> Feb 2001'
//
#include <stdio.h>
#include <iostream>
#include <strstream.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long numpages = 0;
static unsigned long long numbytes = 0;
static struct timeval starttime, lasttime, currenttime;
static int numsites = 0; // Jun 2002

void WriteStatus(void);

void CreateStatusFile(void)
{
   if (gettimeofday(&starttime,NULL) != 0) {
      cerr << "Internal error: " << __FILE__ << ':' << __LINE__ << endl;
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
   ostrstream stat;
   stat << "Crawler PID " << (int) getpid() << "\r\n"
        << "Start seconds " << starttime.tv_sec << "\r\n"
        << "Status seconds " << currenttime.tv_sec << "\r\n"
        << "Bytes saved " << numbytes << "\r\n"
        << "Pages saved " << numpages << "\r\n"
        << "Sites " << numsites << "\r\n";

   lasttime = currenttime;

   stat << ends;
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
      cerr << "Internal error: " << __FILE__ << ':' << __LINE__ 
           << ": number of currently-crawling sites is negative: " << numsites
           << endl;
   }
   return;
}

