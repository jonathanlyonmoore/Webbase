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
#ifndef FETCH_H
#define FETCH_H

#include <sys/socket.h>
#include "url.h"
#include "net_mod.h"

const int RC_FM_ERROR_START                 = -100;
const int RC_FM_INVALID_PARAMETER           = RC_FM_ERROR_START - 1;
const int RC_FM_TOO_MANY_PENDING_REQUEST    = RC_FM_ERROR_START - 2;
const int RC_FM_CANNOT_READ_SOCKET          = RC_FM_ERROR_START - 3;
const int RC_FM_CANNOT_WRITE_SOCKET         = RC_FM_ERROR_START - 4;
const int RC_FM_CANNOT_ALLOCATE_MEMORY      = RC_FM_ERROR_START - 5;
const int RC_FM_TOO_LONG_RESPONSE           = RC_FM_ERROR_START - 6;
const int RC_FM_BEING_TRANSFERRED           = RC_FM_ERROR_START - 7;
const int RC_FM_TIMEOUT                     = RC_FM_ERROR_START - 8;

const int MAGIC_NUMBER  =  0x5A2F33EF;
const int MAX_ARCHIVE_HEADER_SIZE = 10*1024;

const int MAX_PENDING_REQUESTS = 200;
const int INITIAL_BUFFER_SIZE  = 16*1024;
const int MAX_COMMAND_LENGTH   = INITIAL_BUFFER_SIZE;
const int MAX_PAGE_SIZE        = 1*1024*1024;
const int MAX_WAIT             = 5*60000; // wait at most 5 minutes
const int WAIT_UNTIL_CLOSE     = 10000;   // wait 10 seconds before
                                          // closing write connection

#define DEFAULT_METHOD  "GET"
#define DEFAULT_PROTOCOL "HTTP"
#define DEFAULT_VERSION  "1.0"
#define DEFAULT_HTTP_PORT 80


class fetch_module {
   public:
      // public data types
      struct response {
	    int         request_id;
	    int         error_code;
	    const url*  page_url;

	    time_t      crawled_time;
	    int         response_time;

	    const char* page;
	    int         page_size;
      };
      typedef void (*callback_type)(void*, response*);
      struct request {
	    callback_type callback;
	    void*         callback_param;
	    unsigned      server_ip;
	    const url*    page_url;
      };

      // request status
      static const int CONNECTING_TO_SERVER = 100;
      static const int SENDING_REQUEST      = 101;
      static const int WAITING_FOR_RESPONSE = 102;
      static const int READING_RESPONSE     = 103;
      static const int WAITING_FOR_ARCHIVER = 104;
      static const int ARCHIVING_HEADER     = 105;
      static const int ARCHIVING_PAGE       = 106;
      static const int DONE                 = 107;

      // public methods
      fetch_module(net_mod* nm, const net_addr& archiver_addr);
      ~fetch_module();
      
      int request_page(request& req);
      int cancel_request(int request_id);
      int check_request(int request_id);
      int process();

   private:
      // request bookkeeper
      struct table_entry {
	    int            request_id;
	    int            sd;
	    int            status;
	    int            start_time;

	    static_url     url;

	    callback_type  callback;
	    void*          callback_param;

	    char           *buffer;
	    int            buf_cursor;
	    int            buf_size;
      };

      table_entry  *table;
      int   cur_request_id;
      int   num_pending_requests;

      int         archive_sd;
      int        *archive_queue;
      int         archive_queue_size;
      int         archive_queue_cursor;

      char       *archive_buffer;
      int         archive_packet_cursor;
      int         archive_packet_size;

      net_mod*   nm;
      net_addr   archiver_addr;
      
      // helper functions
      int append_request(request& req);
      int finish_request(int entry_num, int return_code);
      int release_resources(int entry_num);

      int send_packet(int entry_num);
      int read_packet(int entry_num);

      int double_buffer_size(int entry_num);
};



#endif // FETCH_H

