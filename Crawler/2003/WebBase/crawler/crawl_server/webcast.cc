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


int init(const char* repName);
int finish();
int outputpage(FILE* pagestream, FILE* linkstream);
static int64 reppos = 0;

int main(int argc, char *argv[])
{
    FILE *links,*pages;
    if (argc < 3) {
       fprintf(stderr,"Usage: %s repository linkscmd pagescmd\n",argv[0]);
       fprintf(stderr,"  where linkscmd is a program that can take the webcat"
	              "link list on stdin\n");
       fprintf(stderr,"  where pagescmd is a program that can take webcat"
	              "pages on stdin\n");
       fprintf(stderr,"Example: %s repository 'cat > /dev/null' 'cat'"
	              "gets normal webcat behavior\n",argv[0]);
       return (1);
    }

    links = popen(argv[2],"w");
    pages = popen(argv[3],"w");
    if (links == NULL) {
       fprintf(stderr,"Cannot open %s for output: ",argv[1]);
       perror("");
       return(2);
    }
    if (pages == NULL) {
       fprintf(stderr,"Cannot open %s for output: ",argv[2]);
       perror("");
       return(2);
    }

    init(argv[1]);

    while (1) {
       while (outputpage(pages, links) >= 0);
       repo->seek(0, SEEK_SET);
    }
    finish();

    fcloseall();
    return(0);
}



repository *repo = NULL;

int init(const char* repName)
{
   repo = new repository(repName);
   if (repo->seek(0, SEEK_SET) < 0) {
      cerr << "Invalid seek position" << endl;
      exit(1);
   }
   
   return 0;
}

int finish()
{
   delete repo;
   return 0;
}

int outputpage(FILE* pagestream, FILE* linkstream)
{
   const  int BUFFER_SIZE = 2*1024*1024;
   static char buffer[BUFFER_SIZE];
   static char url[100*1024];
   static int   pagecount = 0;
   static html_parser parser;

   int  rc, bufSize = BUFFER_SIZE;
   time_t  time;

   // read a page
   if ((rc = repo->read(url, time, buffer, bufSize)) < 0) return rc;
   buffer[bufSize] = 0;

   // print the page itself
   fprintf(pagestream, "==P=>>>>=i===<<<<=T===>=A===<=!Junghoo!==>\n");
   fprintf(pagestream, "URL: %s\nDate: %sPosition: %lld\n\n%s\n", 
	   url, ctime(&time), reppos, buffer);

   // print the links in the page
   int         tag;
   const char  *url_start, *url_end;
   static char linkbuffer[100*1024];
   static_url  base, link;

   fprintf(linkstream, "==P=>>>>=i===<<<<=T===>=A===<=!Junghoo!==>\n");
   fprintf(linkstream, "URL: %s\n", url);

   base.replace(url, url + strlen(url));
   parser.start_parse(buffer, buffer+bufSize);
   while ((tag = parser.get_link(url_start, url_end)) >= 0) {
      if (tag == html_parser::BASE) {
	 static_url temp(base);
	 base.replace(url_start, url_end, temp);
      } else {
	 link.replace(url_start, url_end, base);
	 if (tag == html_parser::AREA || tag == html_parser::A ||
	     tag == html_parser::LINK || tag == html_parser::FRAME ||
	     tag == html_parser::IFRAME) {
	    link.strcpy(linkbuffer, linkbuffer + 100*1024);
	    fprintf(linkstream, "%s\n", linkbuffer);
	 }
      }
   }
   parser.end_parse();

   reppos = repo->tell();

   if (++ pagecount % 10000 == 0) {
      LOG(("%d pages (loc: %lld) %s", pagecount, reppos, ctime(&time)));
   }

   return 0;
}
