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


const int BUFFER_SIZE = 2*1024*1024;
char buffer[BUFFER_SIZE];

int64  startfrom = 0;
int    outputlink = 0;
int    urlonly = 0;
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

    int  rc, bufSize = BUFFER_SIZE, count = 0;
    int64   pos = 0;
    time_t  time;

     // parse command parameters
    parse_opts(argc, argv);

    repo = new repository(repName);
    if (repo->seek(startfrom, SEEK_SET) < 0) {
       cerr << "Invalid seek position" << endl;
       exit(1);
    }

    pos = repo->tell();
    while ((rc = repo->read(url, time, buffer, bufSize)) >= 0) {
       process(url, time, pos, buffer, buffer + bufSize);
       if (++ count % 10000 == 0) {
	  LOG(("%d pages (loc: %lld) %s", count, pos, ctime(&time)));
       }
       if (maxcount >= 0 && count >= maxcount) {
	   break;
       }
       pos = repo->tell();
       bufSize = BUFFER_SIZE;
    }

    return 0;
}


int process(const char* url, time_t time, int64 pos,
	    const char* start, char* end)
{
   static html_parser parser;
   static char url_buffer[100*1024];

   if (!urlonly) {
      printf("==P=>>>>=i===<<<<=T===>=A===<=!Junghoo!==>\n");
   }
   printf("URL: %s\nDate: %sPosition: %lld\n\n", url, ctime(&time), pos);
   if (!outputlink && !urlonly) {
      *end = '\0';
      printf("%s\n", start);
      return 0;
   }
   if (urlonly) {
      return 0;
   }

   int         tag;
   const char  *url_start, *url_end;
   static_url  base, link;

   base.replace(url, url + strlen(url));
   parser.start_parse(start, end);
   while ((tag = parser.get_link(url_start, url_end)) >= 0) {
      if (tag == html_parser::BASE) {
	 static_url temp(base);
	 base.replace(url_start, url_end, temp);
      } else {
	 link.replace(url_start, url_end, base);
	 if (tag == html_parser::AREA || tag == html_parser::A ||
	     tag == html_parser::LINK || tag == html_parser::FRAME ||
	     tag == html_parser::IFRAME) {
	    link.strcpy(url_buffer, url_buffer + 100*1024);
	    printf("%s\n", url_buffer);
	 }
      }
   }
   parser.end_parse();

   return 0;
}
