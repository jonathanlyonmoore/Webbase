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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/times.h>
#include <stdio.h>
#include <fstream.h>
#include <zlib.h>
#include "manager.h"
#include "my_utility.h"
#include "html_parser.h"
#include "robot_parser.h"
#include "crawl_utility.h"

//
// Callback function for fetch_module
// This function is called whenever the crawler got a response
//


static html_parser parser;

static inline bool VALID_PAGE(int err, int size)
{
   return ((err == 0 && size > 0) || (err == RC_FM_TOO_LONG_RESPONSE));
}


#if defined(HISTORY)
static unsigned long
CRC32_BUFFER(const char* buffer, unsigned long acc)
{
   static unsigned long fcstab[] = {
    0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
    0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
    0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
    0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
    0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
    0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
    0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
    0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
    0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
    0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
    0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
    0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
    0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
    0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
    0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
    0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
    0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
    0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
    0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
    0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
    0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
    0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
    0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
    0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
    0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
    0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
    0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
    0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
    0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
    0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
    0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
    0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
    0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
    0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
    0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
    0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
    0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
    0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
    0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
    0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
    0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
    0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
    0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
    0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
    0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
    0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
    0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
    0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
    0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
    0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
    0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
    0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
    0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
    0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
    0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
    0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
    0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
    0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
    0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
    0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
    0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
    0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
    0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
    0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
    };

  register unsigned long fcs = acc;
  register unsigned char c;
  register const char* bp = buffer;

  while ((c = *bp++) != 0) {
     fcs = (((fcs) >> 8) ^ fcstab[((fcs) ^ c) & 0xFF]);
  }
  return (fcs);
}

#endif

void crawl_callback(void* param, fetch_module::response* rsp)
{
  //  cerr << "callback on " << rsp->page_url->path << endl;
   manager::request_info *req = (manager::request_info*)param;
   manager::queue_info &queue_entry 
      = req->mg->queue_info_table[req->queue_entry];

   /* new redirection code starts here */
   int  respCode = 0;
   if (strncasecmp(rsp->page, "HTTP/", 5) == 0) {
	char *code;
        if ((code = strchr(rsp->page + 5, ' ')) == NULL)  goto parse_page;

        respCode = *++code - '0';
        if (respCode == 4 || respCode == 5)  goto store_page;
        if (respCode == 3) {
            //
            // find redirected URL
            //
            int rc;
            int url_count;
            static_url  link;
            char  *link_start, *link_end;
            url_queue::url_info  url_entry;
            url_entry.u       = &link;
            url_entry.level   = req->level;
            url_entry.seed_id = req->seed_id;
            
            
            /* find Location field */
            link_start = strstr(code, "\nLocation");
            if (link_start == NULL)  goto parse_page;
            link_end = strstr(link_start + 9, "http://");
            if (link_end == NULL) {
                link_end = strstr(link_start + 9, "HTTP://");
            }
            if (link_end == NULL)  goto parse_page;
            link_start = link_end;
            link_end = link_start + strcspn(link_start, " \t\v\r\n");
            link.replace(link_start, link_end);
                    
            // in the same server?
            if (!req->mg->is_same_server(link, req->queue_entry)) 
                goto store_page;
                
            // meet filter condition?
            url_count = (url_entry.seed_id > 0) ?
                queue_entry.url_count_per_seed[url_entry.seed_id] + 1: 0;
		 
            rc = queue_entry.filter.check(&link, url_entry.seed_id, 
                                          url_entry.level, url_count);
            if (rc < 0) {
                if (rc == RC_UF_MAX_LEVEL) {
                    queue_entry.status = 
                        manager::queue_info::MAX_LEVEL_REACHED;
                } else if (rc == RC_UF_MAX_COUNT) {
                    queue_entry.status = 
                        manager::queue_info::MAX_COUNT_REACHED;
                }
                goto store_page;
            }

            //
            // insert to the url queue
            //
            if (queue_entry.queue.push_url(url_entry) == 0) {
                queue_entry.url_count_per_seed[url_entry.seed_id]++;
            }

            goto store_page;
        }
   }
   /* New redirection code ends here */
   
 parse_page:
   if (VALID_PAGE(rsp->error_code, rsp->page_size) && 
       req->seed_id != 0 &&
       url_filter::parse_enabled(rsp->page_url)
       ) {
      //
      // extract links and add them to the url_queue
      //

      int         tag;
      const char  *url_start, *url_end;
      static_url  base(*rsp->page_url), link;

      url_queue::url_info  url_entry;
      url_entry.u       = &link;
      url_entry.level   = req->level + 1;
      url_entry.seed_id = req->seed_id;

      parser.start_parse(rsp->page, rsp->page + rsp->page_size);
      while ((tag = parser.get_link(url_start, url_end)) >= 0) {
	 if (tag == html_parser::BASE) {
	    base.replace(url_start, url_end, static_url((url&)base));
	 } else {
	    link.replace(url_start, url_end, base);
	    if (tag == html_parser::AREA || tag == html_parser::A ||
		tag == html_parser::LINK || tag == html_parser::FRAME ||
		tag == html_parser::IFRAME || 
		(url_filter::img_enabled() && tag == html_parser::IMG)
		) {
	       //
	       // check filter conditions
	       //
	       int rc;
	       int url_count;

	       // http?
	       if (strcasecmp(link.scheme, "http") != 0) 
		  continue;

	       // in the same server?
	       if (!req->mg->is_same_server(link, req->queue_entry)) {
		  /* TEMPORARY!!!: For now we do not return new sites
		  if (req->mg->put_aside->put_url(link) < 0) {
		     string new_url_list;
		     req->mg->put_aside->purge_url(new_url_list);
		     new_url_list = "NEW_SITES\r\n" + new_url_list;
		     new_url_list += "\r\n";

		     // send the new urls to the site server
		     req->mg->comm->send_new_site(new_url_list);
		     
		     req->mg->put_aside->put_url(link);
		  }
		  */
		  continue;
	       }

	       // meet filter condition?
	       url_count = (url_entry.seed_id > 0) ?
		  queue_entry.url_count_per_seed[url_entry.seed_id] + 1: 0;
		 
	       rc = queue_entry.filter.check(&link, url_entry.seed_id, 
					      url_entry.level, url_count);
	       if (rc < 0) {
		  if (rc == RC_UF_MAX_LEVEL) {
		     queue_entry.status = 
			manager::queue_info::MAX_LEVEL_REACHED;
		  } else if (rc == RC_UF_MAX_COUNT) {
		     queue_entry.status = 
			manager::queue_info::MAX_COUNT_REACHED;
		  }
		  continue;
	       }

	       //
	       // insert to the url queue
	       //
	       if (queue_entry.queue.push_url(url_entry) == 0) {
		  queue_entry.url_count_per_seed[url_entry.seed_id]++;
	       }
	    }
	 }
      }
      parser.end_parse();
   }

   // if robots.txt has been crawled (seed_id = 0), parse it and add filters.
   if (VALID_PAGE(rsp->error_code, rsp->page_size) && req->seed_id == 0) {
      string          page(rsp->page, rsp->page_size);
      vector<string>  excl;
      parse_robot_file(page, "Pita", excl);
      for (int i = 0; i < (int)excl.size(); i++) {
	 queue_entry.filter.add_prefix_filter(excl[i]);
      }
   }

 store_page:
#if (!defined(NOSAVE)&&!defined(HISTORY))
   //
   // store the page to the repository
   //
   if (VALID_PAGE(rsp->error_code, rsp->page_size) &&
       url_filter::store_enabled(rsp->page_url)) {
       const int   BUFFER_SIZE = 100*1024;
       static char url[BUFFER_SIZE];
       int   rc;
       
       // get url
       rsp->page_url->strcpy(url, url + BUFFER_SIZE);
       if ((rc = req->mg->repo->store(url, rsp->page, rsp->page_size)) < 0) {
	 LOG(("Archiver error %s", url));
	 finish_process();
       }
     }
#endif

#if defined(HISTORY)
   {
      extern FILE* _HISTORYFILE;

      string page(rsp->page, rsp->page_size);
      char     rspcode[4];
      unsigned page_checksum;
      int      tmp;

      const int   URL_BUFFER_SIZE = 10*1024;
      static char url[URL_BUFFER_SIZE];

      time_t     now_t;
      struct tm  now;
      now_t = time(NULL);
      localtime_r(&now_t, &now);

      // get url
      rsp->page_url->strcpy(url, url + URL_BUFFER_SIZE);
      
      // extract response code
      tmp = page.find(' ');
      if ((unsigned)tmp == string::npos) goto history_exit;
      strncpy(rspcode, page.c_str() + tmp + 1, 3);
      rspcode[3] = 0;

      // calculate the page checksum
      tmp = page.find("\r\n\r\n");
      if ((unsigned)tmp == string::npos) goto history_exit;
      page_checksum = CRC32_BUFFER(page.c_str() + tmp + 4,  0xFFFFFFFF);
      
      // store the entry
      fprintf(_HISTORYFILE, "%s %02d:%02d %s %x\n", 
	      url + 7, now.tm_hour, now.tm_min, rspcode, page_checksum);
      fflush(_HISTORYFILE);
   }
  history_exit:
#endif
      
   // delete request entry
   req->request_id = 0;
   req->mg->request_count--;

   // update queue statistics
   queue_entry.pending_request_count--;
   if (VALID_PAGE(rsp->error_code, rsp->page_size)) {
      queue_entry.success_count++;
      queue_entry.consecutive_errors = 0;
      queue_entry.response_time += rsp->response_time;
   } else {
      queue_entry.error_count++;
      queue_entry.consecutive_errors++;
   }


   // check whether we need to try the page again
   if (!VALID_PAGE(rsp->error_code, rsp->page_size)) {
      if (++req->num_trials < MAX_NUM_TRIALS) {
	 int  rc;
	 rc = req->mg->request_page(req->queue_entry, rsp->page_url, 
				    req->seed_id, req->level, req->num_trials);
	 if (rc == 0) {
	    LOG(("Retry: %s%s (trial:%d id:%d seed:%d level:%d status:%d)",
		 rsp->page_url->net_loc, rsp->page_url->path,
		 req->num_trials+1, rsp->request_id, req->seed_id, req->level,
		 rsp->error_code));
	    return;
	 }
      }
   }

   LOG(("Done: %s%s (id:%d seed:%d level:%d status:%d size:%d %d ms)",
        rsp->page_url->net_loc, rsp->page_url->path,
        rsp->request_id, req->seed_id, req->level, rsp->error_code,
        rsp->page_size, rsp->response_time));

   // status file - <wlam@cs.stanford.edu> Feb 2001
   FinishedPage(rsp->page_size);

   return;
}

