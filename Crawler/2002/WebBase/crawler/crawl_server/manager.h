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
#ifndef MANAGER_H
#define MANAGER_H

#include <vector>
#include <string>
#include "url_queue.h"
#include "url_filter.h"
#include "putaside_queue.h"
#include "fetch.h"
#include "crawl_comm.h"
#ifdef JUN_ARCHIVER
#include "archiver.h"
#else
#include "bigfile.h" 
#endif
#include "net_mod.h"

const int MAX_NUM_QUEUES   =  300;
const int MAX_QUEUE_PENDING_REQUESTS =  2;
const int MAX_NUM_TRIALS             =  3;

const int RC_MG_ERROR_START      =  -300;
const int RC_MG_INVALID_QUEUE    = RC_MG_ERROR_START - 1;
const int RC_MG_NO_MORE_QUEUE    = RC_MG_ERROR_START - 2;
const int RC_MG_QUEUE_EMPTY      = RC_MG_ERROR_START - 4;
const int RC_MG_WAIT_TIMER       = RC_MG_ERROR_START - 5;
const int RC_MG_WAIT_RESPONSE    = RC_MG_ERROR_START - 6;
const int RC_MG_TOO_MANY_REQUESTS   = RC_MG_ERROR_START - 7;
const int RC_MG_INVALID_REQUEST     = RC_MG_ERROR_START - 8;
const int RC_MG_INVALID_PARAMETER   = RC_MG_ERROR_START - 9;
const int RC_MG_TOO_MANY_ERRORS     = RC_MG_ERROR_START - 10;
const int RC_MG_INVALID_FILE_FORMAT = RC_MG_ERROR_START - 11;

class manager {
   public:
      manager(int id,
	      const net_addr& local_addr,
	      const net_addr& report_addr,
	      const net_addr& archive_addr);
      ~manager();

      int   request_crawl(const string& packet);
      int   load_queue(const string& packet);
      int   suspend_crawl(int queue_id, ostream& os);
      int   main_loop();
      int   get_num_queues() const;

   private:
      struct queue_info {
	    int   queue_id;
	    vector<string>     site_names;
	    vector<unsigned>   site_ips;
	    url_queue          queue;
	    url_filter         filter;

	    vector<int>        url_count_per_seed;

	    int     delay;
	    time_t  start;
	    int     pending_request_count;
	    int     success_count;
	    int     error_count;
	    int     consecutive_errors;
	    int     response_time;
	    int     last_crawl;

	    int     status;
	    // status 
	    static const int  NORMAL = 0;
	    static const int  MAX_LEVEL_REACHED = 1;
	    static const int  MAX_COUNT_REACHED = 2;
	    static const int  TOO_MANY_ERRORS   = 3;

	    queue_info();
	    ~queue_info();
	    
	    int reset();
	    int save(ostream& out, const char* name = "QueueInfo");
	    int load(istream& in,  const char* name = "QueueInfo");
      };
      struct request_info {
	    int      request_id;
	    int      seed_id;
	    int      level;
	    int      num_trials;
	    int      queue_entry;
	    manager* mg;
	    
	    request_info();
	    ~request_info();
      };

      queue_info   *queue_info_table;
      int          queue_info_table_cursor;
      int          queue_count;

      request_info *request_info_table;
      int          request_info_table_cursor;
      int          request_count;

      putaside_queue  *put_aside;
      net_mod         *nm;
      fetch_module    *fm;
      crawl_comm      *comm;
#ifdef JUN_ARCHIVER
      archiver        *repo;
#else
      repository      *repo;  
#endif /* JUN_ARCHIVER */
      net_addr         local_addr;
      int              crawler_id;

      int          cur_queue_id;

#ifdef SOCKET_CONTROL
      bool             paused;
/* obsolete hack
 #define RUNNING (0)
 #define PAUSED  (1)
      int             state;
      vector<const char *>  paused_queue_buf; 
      vector<int>     paused_queue_len; 
*/
#endif

      int  request_page(int queue_entry_num, const url* u,
			int seed_id, int level, int num_trials = 0);

      int  select_queue(int& queue_entry, int& time);
      int  delete_queue(int queue_entry);

      int  is_same_server(const url& u, int queue_num) const;
      unsigned  get_server_ip(int queue_entry) const;

      friend void crawl_callback(void* param, fetch_module::response* rsp);
};

#endif // MANAGER_H
