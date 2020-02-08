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
#include <stdio.h>
#include <unistd.h>
#include "bigfile.h"
#include "my_utility.h"
#include "html_parser.h"
#include "url.h"

int process(const char* url, time_t time, int64 pos,
	    const char* start, char* end);
void parse_opts(int argc, char* argv[]);



int64  startfrom = 0;
int    outputlink = 0;
int    urlonly = 1;
int    maxcount = -1;
const char* repName;

void parse_opts(int argc, char* argv[])
{
   extern char *optarg;
   extern int   optind;

   int c, errflg = 0;

   while ((c = getopt(argc, argv, "uls:c:")) != EOF) {
      switch (c) {
	 case 'c':
            sscanf(optarg, "%d", &maxcount);
	    break;
 	 case 'u':
	    urlonly++;
	    break;
	 case 'l':
	    outputlink++;
	    break;
	 case 's':
	    sscanf(optarg, "%lld", &startfrom);
	    break;
	 case '?':
	    errflg++;
      }
   }

   if (optind != argc - 1) errflg++;
   repName = argv[optind];
   
   if (errflg) {
      fprintf(stderr, "Usage: %s [-lu] [-s <startpos>] [-c <pagecount>] <repository>\n", argv[0]);
      exit(1);
   }

   return;
}

int main(int argc, char* argv[])
{
    repository *repo;
    char url[100*1024];

    int  rc, count = 0, pagesize;
    int64   totalsize = 0, pagecount = 0;
    int64   pos = 0;
    time_t  time;
    struct tm *stime;

     // parse command parameters
    parse_opts(argc, argv);

    repo = new repository(repName);
    if (repo->seek(startfrom, SEEK_SET) < 0) {
       cerr << "Invalid seek position" << endl;
       exit(1);
    }

    pos = repo->tell();
    while ((rc = repo->getheader(url, time, pagesize)) >= 0) {
        stime = localtime(&time);
        if (stime->tm_mday == 8 && stime->tm_mon == 0) {
            fprintf(stdout, "%s\n", url);
            totalsize += pagesize;
            pagecount++;
        }
        if (++ count % 10000 == 0) {
            LOG(("%d pages (loc: %lld, totalsize: %lld)", 
                 count, pos, totalsize));
        }
        if (maxcount >= 0 && count >= maxcount) {
            break;
        }
        pos = repo->tell();
    }
    LOG(("%lld pages downloaded. %lld bytes in total",
         pagecount, totalsize));
    
    return 0;
}
