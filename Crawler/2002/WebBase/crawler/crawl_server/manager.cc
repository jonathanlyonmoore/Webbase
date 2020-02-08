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
#include <limits.h>
#include <sys/times.h>
#include <stdio.h>
#include <fstream.h>
#include <zlib.h>
#include <strstream>
#include <errno.h>
#include "my_utility.h"
#include "html_parser.h"
#include "readmsg.h"
#include "manager.h"
#include "crawl_utility.h"

//
// primitive indexer
//

extern vector<string>* disklist;
extern char repName[];

void crawl_callback(void* param, fetch_module::response* rsp);

manager::request_info::request_info()
   : request_id(0)
{}

manager::request_info::~request_info()
{}

manager::manager(int id, 
		 const net_addr& local_addr,
		 const net_addr& report_addr, 
		 const net_addr& archive_addr)
{
   crawler_id   = id;
   cur_queue_id = 0;

   queue_info_table = new queue_info[MAX_NUM_QUEUES];
   queue_count = 0;
   queue_info_table_cursor = 0;
   
   request_info_table = new request_info[MAX_PENDING_REQUESTS];
   request_count = 0;
   request_info_table_cursor = 0;

   this->local_addr = local_addr;

#if defined(STANFORD)
   put_aside = new putaside_queue(100);
#else
   put_aside = new putaside_queue(1000);
#endif
   nm        = new net_mod();
   fm        = new fetch_module(nm, archive_addr);
   comm      = new crawl_comm(nm, local_addr, report_addr);
#if !defined(NOSAVE)
#ifdef JUN_ARCHIVER
   repo      = new archiver('a');
#else
   repo      = new repository(repName, true, disklist, 'a'); 
#endif 
#endif

#ifdef SOCKET_CONTROL
   /* state     = RUNNING; */
   paused    = false;
#endif
}

manager::~manager()
{
   if (comm != NULL) {
      delete comm;
   }
   if (fm != NULL) {
      delete fm;
   }
   if (nm != NULL) {
      delete nm;
   }
#if !defined(NOSAVE)
   if (repo != NULL) {
      delete repo;
   }
#endif
   if (put_aside != NULL) {
      delete put_aside;
   }
   if (queue_info_table != NULL) {
      delete [] queue_info_table;
   }
   if (request_info_table != NULL) {
      delete [] request_info_table;
   }
}


int manager::request_crawl(const string& request)
{
   int  entry_num;
   vector<string> request_lines;

   string               seed_string;
   static_url           seed_url;

   split_string(request, "\r\n", request_lines);

   // check the validity of the request
   if (request_lines.size() != 4 && request_lines.size() != 3) {
      return RC_MG_INVALID_REQUEST;
   }

   // check whether the request can be handled
   if (queue_count >= MAX_NUM_QUEUES) {
      return RC_MG_TOO_MANY_REQUESTS;
   }

   // find an empty slot from the queue table
   for (entry_num = 0; entry_num < MAX_NUM_QUEUES; entry_num++) {
      if (queue_info_table[entry_num].queue_id == 0) {
	 break;
      }
   }
   assert(queue_info_table[entry_num].queue_id == 0);
   queue_info &entry = queue_info_table[entry_num];

   // log start time
   entry.start = time(NULL);

   //
   // parse request
   //
   vector<string> request_fields;

   // extract site names
   split_string(request_lines[0], " ", request_fields);
   if (request_fields.size() < 1)  goto invalid_request;
   for (int i = 0; i < (int)request_fields.size(); i++) {
      entry.site_names.push_back(request_fields[i].c_str());
   }

   // extract ips
   split_string(request_lines[1], " ", request_fields);
   if (request_fields.size() < 1)  goto invalid_request;
   for (int i = 0; i < (int)request_fields.size(); i++) {
      unsigned ip = net_mod::inet_addr(request_fields[i]);
      if (ip == (unsigned)-1)  goto invalid_request;
      entry.site_ips.push_back(ip);
   }

   // extract max_level, max_count, delay
   int max_level, max_count;

   split_string(request_lines[2], " ", request_fields);
   if (request_fields.size() != 3)  goto invalid_request;
   max_level = atoi(request_fields[0].c_str());
   max_count = atoi(request_fields[1].c_str());
   entry.delay = atoi(request_fields[2].c_str());

   // extract filters
   if (request_lines.size() == 4) {
      split_string(request_lines[3], " ", request_fields);
      for (int i = 0; i < (int)request_fields.size(); i++) {
	 entry.filter.add_prefix_filter(request_fields[i]);
      }
   }

   // insert seeds to the queue

   url_queue::url_info  seed_url_info;

/*#if !defined(NOSAVE)&&!defined(HISTORY) */
   // crawl and save robots.txt file
   seed_string = "http://" + entry.site_names[0] + "/robots.txt";
   seed_url.replace(seed_string.data(), 
		    seed_string.data() + seed_string.size());
   seed_url_info.u       = &seed_url;
   seed_url_info.seed_id = 0;
   seed_url_info.level   = 0;
   entry.queue.push_url(seed_url_info);
/*#endif */

   seed_string = "http://" + entry.site_names[0] + "/";
   seed_url.replace(seed_string.data(), 
		    seed_string.data() + seed_string.size());

   seed_url_info.u       = &seed_url;
   seed_url_info.seed_id = 1;
   seed_url_info.level   = 0;
   entry.queue.push_url(seed_url_info);

   // insert url counter
   entry.url_count_per_seed.push_back(0);
   entry.url_count_per_seed.push_back(0);

   

   // add max_count, max_level count filter
   entry.filter.add_seed_filter(1, max_level, max_count);

   entry.queue_id = ++cur_queue_id;
   entry.status   = queue_info::NORMAL;
   queue_count++;

   LOG(("SiteRequest: %s (id: %d active_count: %d)",
	entry.site_names[0].c_str(), cur_queue_id, queue_count));

   return cur_queue_id;

 invalid_request:
   
   entry.reset();
   return RC_MG_INVALID_REQUEST;
}


int manager::suspend_crawl(int queue_id, ostream& os)
{
   int  rc;
   int  entry_num;

   if (queue_id <= 0 || queue_id > cur_queue_id) {
      return RC_MG_INVALID_PARAMETER;
   }

   // find the queue entry
   for (entry_num = 0; entry_num < MAX_NUM_QUEUES; entry_num++) {
      if (queue_info_table[entry_num].queue_id == queue_id) {
	 break;
      }
   }

   queue_info& entry = queue_info_table[entry_num];

   if (entry.queue_id != queue_id) {
      return RC_MG_INVALID_PARAMETER;
   }
   
   // cancel all pending requests
   for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
      request_info& req = request_info_table[i];
      if (req.request_id != 0 && req.queue_entry == entry_num) {
	 rc = fm->cancel_request(req.request_id);
	 req.request_id = 0;
	 request_count--;
      }
   }
   assert(request_count >= 0);
   
   // save queue information
   entry.save(os);
   os.flush();

   // delete queue
   LOG(("SiteDone: %s (id: %d count: %d status: Suspend)",
	entry.site_names[0].c_str(), entry.queue_id, queue_count-1));

   rc = delete_queue(entry_num);
   assert(rc == 0);
   
   return 0;
}

   
int manager::main_loop()
{
   int  rc;
   int  wait_until = -1;
   int  queue_entry_num;
   string packet;

   LOG(("Starting Crawler %d", crawler_id));

 main_loop:
   /*
    *  update status of sockets
    */
   nm->poll(wait_until);
   comm->process();
   fm->process();


   /*
    * get command from site manager
    */
   rc = comm->get_request(packet);
   if (rc == 0) {
      string   command;
      string   response;

      // read command
      {
	 int command_end = packet.find("\r\n");
	 command = packet.substr(0, command_end - 0);
	 if (command_end != (int)string::npos) command_end += 2;
	 packet.erase(0, command_end);
      }

      if (command.compare("CRAWL") == 0) {
	 rc = request_crawl(packet);
	 ostrstream  respstr;

	 if (rc >= 0) {
            // status file - <wlam@cs.stanford.edu> Feb 2001
            IncrementSites();

	    writemsg(respstr, "Status", "Ok");
	    respstr << endl << ends;
	 } else {
	    writemsg(respstr, "Status", "Error");
	    writemsg(respstr, "Desc", itoa(rc));
	    respstr << endl << ends;
	 }

	 response.assign(respstr.str(), respstr.pcount());
	 respstr.freeze(0);

      } else if (command.compare("LOAD") == 0) {
	 // check whether the request can be handled
	 rc = load_queue(packet);
	 if (rc >= 0) {
            // status file - <wlam@cs.stanford.edu> Feb 2001
            IncrementSites();

	    response = "OK ";
	    response += itoa(rc);
	    response += "\r\n\r\n";
	 } else {
	    response = "ERROR ";
	    response += itoa(rc);
	    response += "\r\n\r\n";
	 }
      } else if (command.compare("SUSPEND") == 0) {
	 ostrstream  info;
	 int queue_id = atoi(packet.c_str());
	 LOG(("SuspendRequest: (id: %d)", queue_id));
	 rc = suspend_crawl(queue_id, info);
	 if (rc == 0) {
            // status file - <wlam@cs.stanford.edu> Feb 2001
            DecrementSites();

	    const char* buffer = info.str();
	    response = "OK\r\n";
	    response.append(buffer, info.pcount());
	    response += "\r\n";
	    free((void*)buffer);
	    LOG(("SuspendDone: Successful (id: %d)", queue_id));
	 } else {
	    response = "ERROR ";
	    response += itoa(rc);
	    response += "\r\n\r\n";
	    LOG(("SuspendDone: Error %d(id: %d)", rc, queue_id));
	 }
      } else if (command.compare("DIE") == 0) {
	 LOG(("Finishing crawl server..."));
	 return 0;
      } else if (command.compare("STATUS") == 0) {
	 // status file - <wlam@cs.stanford.edu> Feb 2001
         char *answer = ReportStatus();
         response = "OK\r\n";
         response += answer;
#ifdef SOCKET_CONTROL
         response += "Status ";
	 response += paused ? "paused" : "running";
	 response += "\r\n";
#endif
	 response += "\r\n";
	 // Status can be polled, so logging can become denial-of-service
	 // LOG(("Status: Successful"));
#ifdef SOCKET_CONTROL
	 // The start/stop commands ("RESUME"/"PAUSE") are idempotent.
      } else if (command.compare("PAUSE") == 0) {
         paused = true;
         LOG(("PauseRequest: Stopped new requests"));
/* obsolete hack
         LOG(("PauseRequest: Suspending all queues"));
	 for (int q_id=1; q_id<=cur_queue_id; ++q_id) {
            ostrstream info;
            int rc = suspend_crawl(q_id, info);
            if (rc == 0) {
               const char* buffer = info.str();
               int buflen = info.pcount();
               paused_queue_buf.push_back(buffer);
               paused_queue_len.push_back(buflen);
               // LOG(("SuspendDone: Successful (id: %d)", q_id));
            } else {
               LOG(("PauseRequest: Queue lost (id: %d)", q_id));
               // LOG(("SuspendDone: Error %d(id: %d)", rc, q_id));
            }
         }
         state = PAUSED;
*/
	 response = "Status: OK\r\n\r\n";
      } else if (command.compare("RESUME") == 0) {
         paused = false;
         LOG(("ResumeRequest: Restoring new requests"));
/* obsolete hack
         LOG(("ResumeRequest: restoring all queues"));
	 for (unsigned int i=0; i<paused_queue_buf.size(); ++i) {
	    string info(paused_queue_buf[i],paused_queue_len[i]);
	    if (load_queue(info) < 0) {
	       LOG(("ResumeRequest: Queue lost"));
	    }
	 }
	 paused_queue_buf.clear();
	 paused_queue_len.clear();
	 state = RUNNING;
*/
	 response = "Status: OK\r\n\r\n";
#endif
      } else {
	 response = "ERROR INVALID_REQUEST\r\n\r\n";
      }

      // return response
      comm->send_response(response);
   }

   if (request_count >= MAX_PENDING_REQUESTS) {
      wait_until = -1;
      goto main_loop;
   }
	    
   if (queue_count <= 0) {
      /* !!!!TEMPORARY: For now, we do not return new sites 
      if (put_aside->get_size() > 0) {
	 string new_url_list;
	 
	 put_aside->purge_url(new_url_list);
	 new_url_list = "NEW_SITES\r\n" + new_url_list;
	 new_url_list += "\r\n";
	 
	 // send the new urls to the site server
	 comm->send_new_site(new_url_list);
      }
      */

      wait_until = -1;
      goto main_loop;
   }


 queue_select_loop:

   rc = select_queue(queue_entry_num, wait_until);
   switch (rc) {
      case RC_MG_NO_MORE_QUEUE:
	 break;
	 
      case RC_MG_QUEUE_EMPTY:
      case RC_MG_TOO_MANY_ERRORS:
	 /*
	  * save statistics
	  */
	 
	 /*
	  * create the report to the site manager
	  */
	 
	 // send report
	 {
#ifndef HISTORY
	    ostrstream  reportstr;
#endif /* HISTORY */
	    queue_info& entry = queue_info_table[queue_entry_num];
	    const char* status = "Done";

	    if (rc == RC_MG_QUEUE_EMPTY) {
	       if (entry.status == queue_info::MAX_LEVEL_REACHED) {
		  status = "Max Level";
	       }
	       if (entry.status == queue_info::MAX_COUNT_REACHED) {
		  status = "Max Count";
	       }
	    }
	    if (rc == RC_MG_TOO_MANY_ERRORS) {
	       entry.status = queue_info::TOO_MANY_ERRORS;
	       status = "Too many Errors";
	    }
	    
#ifndef HISTORY
	    writemsg(reportstr, "Type", "Report");
	    writemsg(reportstr, "Status", status);
	    writemsg(reportstr, "SiteName", entry.site_names[0]);
	    writemsg(reportstr, "CrawlerId", itoa(crawler_id));
	    reportstr << endl;

	    string report(reportstr.str(), reportstr.pcount());
	    reportstr.freeze(0);
	    comm->send_report(report);
#endif /* HISTORY */	  

            // status file - <wlam@cs.stanford.edu> Feb 2001
            DecrementSites();

	    LOG(("SiteDone: %s (id: %d count: %d status: %s)",
		 entry.site_names[0].c_str(), entry.queue_id, queue_count-1,
		 status));
	 }
	 rc = delete_queue(queue_entry_num);
	 assert(rc == 0);
	       
	 
	 /*
	  * report to the site manager
	  */
	 goto queue_select_loop;
	 
      case RC_MG_WAIT_RESPONSE:
      case RC_MG_WAIT_TIMER:
	 break;

      case 0: 
	if (paused) { 
	  wait_until = -1;
	} else {
	    const url_queue::url_info  *info;

	    wait_until = 0;
	    
	    // check whether there is room for another request
	    if (request_count >= MAX_PENDING_REQUESTS) {
	       wait_until = -1;
	       break;
	    }
	    
	    // get the next url to crawl
	    info   = &queue_info_table[queue_entry_num].queue.top_url();
	    
	    // request the page
	    if ((rc = request_page(queue_entry_num, info->u, info->seed_id,
				   info->level)) < 0) {
	       LOG(("Done: %s%s Request error (%d, %s)", 
		    info->u->net_loc, info->u->path, rc, strerror(errno)));
	    }
	    
	    // remove the requested url from the queue
	    queue_info_table[queue_entry_num].queue.pop_url();
	}
	break;
   }

   goto main_loop;
}

int manager::delete_queue(int entry_num)
{
   int  rc;
   queue_info& entry = queue_info_table[entry_num];

   // cancel all pending requests
   for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
      request_info& req = request_info_table[i];
      if (req.request_id != 0 && req.queue_entry == entry_num) {
	 rc = fm->cancel_request(req.request_id);
	 req.request_id = 0;
	 request_count--;
      }
   }
   assert(request_count >= 0);

   // delete url_queue
   entry.reset();
   
   // decrease queue count
   queue_count--;

   return 0;
}
   
int manager::select_queue(int& entry_num, int& crawl_time)
{
   if (queue_count <= 0) {
      crawl_time = -1;
      return RC_MG_NO_MORE_QUEUE;
   }

   int  nonempty_queue_count = 0;
   int  next_request_time;
   int  now = nm->get_time();
   
   crawl_time = INT_MAX;
   entry_num = queue_info_table_cursor;
   do {
      // check whether this entry is valid
      entry_num = (entry_num+1) % MAX_NUM_QUEUES;
      queue_info& entry = queue_info_table[entry_num];
      if (entry.queue_id <= 0)  continue;
      
      // check whether this queue has encountered too many errors
      if ((entry.error_count + entry.success_count >= 10 &&
	  entry.success_count < entry.error_count) ||
	  entry.consecutive_errors >= 5) {
	 queue_info_table_cursor = entry_num;
	 entry_num = entry_num;
	 crawl_time = 0;
	 return RC_MG_TOO_MANY_ERRORS;
      }

      // check whether this queue is empty 
      if (entry.queue.is_empty()) {
	 if (entry.pending_request_count <= 0) {
	    queue_info_table_cursor = entry_num;
	    entry_num = entry_num;
	    crawl_time = 0;
	    return RC_MG_QUEUE_EMPTY;
	 }
	 continue;
      }

      nonempty_queue_count++;

      // check whether we crawled from this queue just before
      next_request_time = nm->time_add(entry.last_crawl, entry.delay);
      if (next_request_time <= now) {
	 // check whether there are too many pending requests for it
	 if (entry.pending_request_count < MAX_QUEUE_PENDING_REQUESTS) {
	    queue_info_table_cursor = entry_num;
	    entry_num = entry_num;
	    crawl_time = 0;
	    return 0;
	 }
	 continue;
      }

      crawl_time = min(crawl_time, next_request_time);
   } while (entry_num != queue_info_table_cursor);

   if (nonempty_queue_count > 0 && crawl_time < INT_MAX) {
      return RC_MG_WAIT_TIMER;
   } else {
      crawl_time = -1;
      return RC_MG_WAIT_RESPONSE;
   }
}

unsigned manager::get_server_ip(int queue_entry_num) const
{
   return queue_info_table[queue_entry_num].site_ips[0];
}

int manager::get_num_queues() const
{
   return queue_count;
}

int manager::is_same_server(const url& u, int queue_num) const
{
   queue_info &queue_entry = queue_info_table[queue_num];

   for (int i = 0; (unsigned)i < queue_entry.site_names.size(); i++) {
      const char* site_name = queue_entry.site_names[i].c_str();
      if (strcasecmp(u.net_loc, site_name) == 0) return 1;
   }

   return 0;
}


int manager::request_page(int queue_entry_num, const url* u,
			  int seed_id, int level, int num_trials)
{
   int  req_entry_num, request_id;
   fetch_module::request  req;

   /* static_url  cannonical_url; */
   //const char* site_name;

   // check whether there is room for another request
   assert(request_count < MAX_PENDING_REQUESTS);

   // find an empty slot in the request table
   req_entry_num = request_info_table_cursor;
   do {
      req_entry_num = (req_entry_num + 1) % MAX_PENDING_REQUESTS;
   } while (request_info_table[req_entry_num].request_id != 0 &&
	    req_entry_num != request_info_table_cursor);
   assert(request_info_table[req_entry_num].request_id == 0);
	 
   /*  We used to substitute the site name with the cannonical name,
       but we encountered many problems because of redirection.
       Now we keep the original URL as it appears on the crawled page 
   // canonize url
   site_name = queue_info_table[queue_entry_num].site_names[0].c_str();
   cannonical_url.replace(u->scheme, site_name, u->port, 
			  u->path, u->param, u->query, u->fragment);
   */

   // send request
   req.server_ip      = get_server_ip(queue_entry_num);
   req.page_url       = u;
   /* req.page_url       = &cannonical_url; */
   req.callback       = crawl_callback;
   req.callback_param = (void*)&request_info_table[req_entry_num];
   
   request_id = fm->request_page(req);
   if (request_id <= 0) {
      queue_info_table[queue_entry_num].last_crawl = nm->get_time();
      queue_info_table[queue_entry_num].error_count++;
      queue_info_table[queue_entry_num].consecutive_errors++;
      return request_id;
   }
	    
   // record request information in the table
   request_info_table[req_entry_num].request_id  = request_id;
   request_info_table[req_entry_num].num_trials  = num_trials;
   request_info_table[req_entry_num].seed_id     = seed_id;
   request_info_table[req_entry_num].level       = level;
   request_info_table[req_entry_num].queue_entry = queue_entry_num;
   request_info_table[req_entry_num].mg          = this;
   request_info_table_cursor = req_entry_num;
   request_count++;

   // update queue statistics
   queue_info_table[queue_entry_num].pending_request_count++;
   queue_info_table[queue_entry_num].last_crawl = nm->get_time();

   return 0;
}


int manager::load_queue(const string& packet)
{	    
   int  entry_num, rc;

   if (queue_count >= MAX_NUM_QUEUES)  return RC_MG_TOO_MANY_REQUESTS;

   // find an empty slot from the queue table
   for (entry_num = 0; entry_num < MAX_NUM_QUEUES; entry_num++) {
      if (queue_info_table[entry_num].queue_id == 0) {
	 break;
      }
   }
   assert(queue_info_table[entry_num].queue_id == 0);
   queue_info &entry = queue_info_table[entry_num];
	    
   // load the packet
   istrstream is(packet.data(), packet.length());
   rc = entry.load(is);
   if (rc < 0) {
      entry.reset();
      return rc;
   }

   // log start time
   entry.start = time(NULL);
   entry.queue_id = ++cur_queue_id;
   entry.status   = queue_info::NORMAL;
   queue_count++;

   LOG(("LoadRequest: %s (id: %d active_count: %d)",
	entry.site_names[0].c_str(), cur_queue_id, queue_count));

   return cur_queue_id;
}


manager::queue_info::queue_info()
{
   queue_id = 0;
   delay = 0;
   start = 0;
   pending_request_count = 0;
   success_count = 0;
   error_count = 0;
   consecutive_errors = 0;
   response_time = 0;
   last_crawl = 0;
   status = NORMAL;
}

manager::queue_info::~queue_info()
{
   site_names.erase(site_names.begin(), site_names.end());
   site_ips.erase(site_ips.begin(), site_ips.end());
   url_count_per_seed.erase(url_count_per_seed.begin(),
			    url_count_per_seed.end());
}

int manager::queue_info::reset()
{
   queue_id = 0;

   site_names.erase(site_names.begin(), site_names.end());
   site_ips.erase(site_ips.begin(), site_ips.end());   
   queue.reset();
   filter.reset();
   url_count_per_seed.erase(url_count_per_seed.begin(), 
			    url_count_per_seed.end());
 
   delay = 0;
   start = 0;
   pending_request_count = 0;
   success_count = 0;
   error_count = 0;
   consecutive_errors = 0;
   response_time = 0;
   last_crawl = 0;
   status = NORMAL;

   return 0;
}

int manager::queue_info::save(ostream& os, const char* name)
{
   os << name << "Start" << endl;

   writelist(os, "SiteNames", site_names);
   {
      vector<string> temp_ips;
      for (int i = 0; i < (int)site_ips.size(); i++) {
	 temp_ips.push_back(net_mod::inet_ntoa(site_ips[i]));
      }
      writelist(os, "SiteIps", temp_ips);
   }
   queue.save(os, "UrlQueue");
   filter.save(os, "UrlFilter");
   writelist(os, "UrlsPerSeed", url_count_per_seed);

   // save put_aside queue
   
   // save values
   writeval(os, "Delay", delay);
   writeval(os, "Error", error_count);
   if (success_count > 0) {
      writeval(os, "Success", success_count);
      writeval(os, "ResponseTime", response_time / success_count);
   }

   os << name << "End" << endl;
   
   return 0;
}

int manager::queue_info::load(istream& is, const char* name)
{
   const int   maxline = 2*1024;
   char        line[maxline];
   string      start, end;

   // initialize data
   reset();

   // start and end marker
   start = name; start += "Start";
   end   = name; end   += "End";

   // skip start marker
   is >> ws; readline(is, line, maxline);
   if (strcmp(line, start.c_str()) == 0) {
      is >> ws; readline(is, line, maxline);
   }

   while (!is.eof()) {
      if (strcmp("SiteNamesStart", line) == 0) {
	 if (readlist(is, "SiteNames", site_names) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("SiteIpsStart", line) == 0) {
	 vector<string> temp_ips;
	 if (readlist(is, "SiteIps", temp_ips) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
	 for (int i = 0; i < (int)temp_ips.size(); i++) {
	    site_ips.push_back(net_mod::inet_addr(temp_ips[i]));
	 }
      } else if (strcmp("UrlQueueStart", line) == 0) {
	 if (queue.load(is, "UrlQueue") < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("UrlFilterStart", line) == 0) {
	 if (filter.load(is, "UrlFilter") < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("UrlsPerSeedStart", line) == 0) {
	 if (readlist(is, "UrlsPerSeed", url_count_per_seed) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("DelayStart", line) == 0) {
	 if (readval(is, "Delay", delay) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("SuccessStart", line) == 0) {
	 if (readval(is, "Success", success_count) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("ErrorStart", line) == 0) {
	 if (readval(is, "Error", error_count) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp("ResponseTimeStart", line) == 0) {
	 if (readval(is, "ResponseTime", response_time) < 0) {
	    return RC_MG_INVALID_FILE_FORMAT;
	 }
	 response_time *= success_count;
      } else if (strcmp(line, end.c_str()) == 0) {
	 return 0;
      }

      is >> ws; readline(is, line, maxline);
   }

   return RC_MG_INVALID_FILE_FORMAT;
}
