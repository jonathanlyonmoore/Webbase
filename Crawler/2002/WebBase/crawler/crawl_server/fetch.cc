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
#include <stdlib.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <sys/times.h>
#include <limits.h>
#include "fetch.h"
#include "errors.h"
#include "my_utility.h"


fetch_module::fetch_module(net_mod* nm, const net_addr& archiver_addr) 
   : cur_request_id(0), num_pending_requests(0), nm(nm)
{ 
   LOG(("Fetch module started......................."));
   signal(SIGPIPE, SIG_IGN); 

   
   // connect to archiver and prepare for it.
   this->archiver_addr = archiver_addr;
   archive_buffer = new char[MAX_ARCHIVE_HEADER_SIZE];
   archive_queue = new int[MAX_PENDING_REQUESTS];
   archive_queue_size = 0;
   archive_queue_cursor = 0;
   /*
   archive_sd = nm->connect(archiver_addr);
   if (archive_sd < 0) { 
      LOG(("Error: cannot contact archiver.."));
      finish_process();
   }
   */

   table        = new table_entry[MAX_PENDING_REQUESTS];

   for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
      table[i].request_id = 0;
      table[i].sd         = -1;
      table[i].callback   = NULL;
      table[i].callback_param = NULL;
      table[i].buffer         = NULL;
      table[i].buf_cursor     = 0;
      table[i].buf_size       = 0;
   }
   
   
}

fetch_module::~fetch_module()
{
   // release resources
   for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
      if (table[i].request_id != 0) {
	 release_resources(i);
      }
   }

   // delete request table
   if (table != NULL) {
      delete [] table;
   }

   // release archive related resources
   if (archive_sd >= 0) nm->close(archive_sd);
   if (archive_queue != NULL) {
      delete [] archive_queue;
   }
   if (archive_buffer != NULL) {
      delete [] archive_buffer;
   }
}

int fetch_module::cancel_request(int request_id)
{
   int  entry_num = request_id % MAX_PENDING_REQUESTS;
   if(table[entry_num].request_id != request_id) {
      return 0;
   }
   
   // release the allocated resources
   release_resources(entry_num);
   table[entry_num].request_id = 0;
   num_pending_requests--;

   return 0;
}

int fetch_module::check_request(int request_id)
{
   int  entry_num = request_id % MAX_PENDING_REQUESTS;
   if (table[entry_num].request_id != request_id) {
      return DONE;
   }
   
   return table[entry_num].status;
}

int fetch_module::request_page(request& req)
{
   // check whether too many requests are pending
   if (num_pending_requests >= MAX_PENDING_REQUESTS) {
      return RC_FM_TOO_MANY_PENDING_REQUEST;
   }

   // check parameter
   if (req.callback == NULL || req.page_url == NULL ||
       (req.page_url->scheme != NULL 
	&& strcasecmp(req.page_url->scheme, "http") != 0)) {
      return RC_FM_INVALID_PARAMETER;
   }

   // save the request information
   return append_request(req);
}

   
int fetch_module::process()
{
   int  rc;
   int  kill_before = net_mod::get_time(-MAX_WAIT);
   int  now = net_mod::get_time();

   nm->poll(0);

   for (int i = 0; i < MAX_PENDING_REQUESTS; i++) {
      if (table[i].request_id == 0) continue;
      if (table[i].start_time < kill_before) {
         finish_request(i, RC_FM_TIMEOUT);
         continue;;
      }
      switch (table[i].status) {
	 case CONNECTING_TO_SERVER:
	 case SENDING_REQUEST:
	    if (nm->ready(table[i].sd)) {
	       rc = send_packet(i);
	       if (rc == RC_FM_BEING_TRANSFERRED) {
		  table[i].status = SENDING_REQUEST;
	       } else if (rc == 0) {
		  nm->set_read(table[i].sd);
		  table[i].status = WAITING_FOR_RESPONSE;
	       } else if (rc < 0) {
		  finish_request(i, rc);
	       }
	    }
	    break;

	 case WAITING_FOR_RESPONSE:
           if (!nm->ready(table[i].sd)) {
               if (nm->time_diff(table[i].start_time, now) >= WAIT_UNTIL_CLOSE)
               {
                   // close the write direction of the socket,
                   // in case the server is waiting for it.
                   LOG(("CloseSocket: %s", table[i].url.net_loc));
                   nm->close(table[i].sd, net_mod::WRITE);
                   table[i].status = READING_RESPONSE;
               }
               break;
           }
           table[i].status = READING_RESPONSE;

	 case READING_RESPONSE:
	    if (nm->ready(table[i].sd)) {
	       rc = read_packet(i);
               if (rc != RC_FM_BEING_TRANSFERRED && rc < 0) {
		  finish_request(i, rc);
	       } else if (rc == 0) {
		  finish_request(i, 0);
		  /*
		  assert(archive_queue_size < MAX_PENDING_REQUESTS);

		  // add the crawled page to the archive queue
		  int  queue_pos;
		  queue_pos = archive_queue_cursor + archive_queue_size;
		  queue_pos %= MAX_PENDING_REQUESTS;
		  archive_queue[queue_pos] = i;
		  archive_queue_size++;

		  table[i].status = WAITING_FOR_ARCHIVER;
		  */
	       }
	    }
	    break;
	 default:
	    break;
      }
   }

   /*
   // send crawled pages to the archiver
   if (archive_queue_size > 0) {
      table_entry &req = table[archive_queue[archive_queue_cursor]];
      int  status;

      switch (req.status) {
	 case WAITING_FOR_ARCHIVER: {
	    //
	    // Format :
	    //
	    // MAGIC_NUMBER (4 bytes)
            // URL length   (4 bytes)
	    // URL
	    // page size    (4 bytes)
	    // page

	    int url_len;
	    
	    memcpy(archive_buffer + 0, &MAGIC_NUMBER, 4);
	    url_len = req.url.strcpy(archive_buffer + 8, 
			 archive_buffer + MAX_ARCHIVE_HEADER_SIZE - 4);
	    memcpy(archive_buffer + 4, &url_len, 4);
	    memcpy(archive_buffer + 8 + url_len, &req.buf_cursor, 4);
	    
	    archive_packet_cursor = 0;
	    archive_packet_size = url_len + 12;
	    req.status = ARCHIVING_HEADER;
	 }
	 case ARCHIVING_HEADER:
	 case ARCHIVING_PAGE:   {
	    int  bytes_written, bytes_to_write;
	    char *packet;

	    if (!nm->ready(archive_sd)) break;

	    packet = (req.status == ARCHIVING_HEADER) ?
	       archive_buffer : req.buffer;
	    
	    bytes_to_write = archive_packet_size - archive_packet_cursor;
	    bytes_written = nm->write(archive_sd, 
				      packet + archive_packet_cursor,
				      bytes_to_write);
	    if (bytes_written == bytes_to_write) {
	       if (req.status == ARCHIVING_HEADER) {
		  archive_packet_cursor = 0;
		  archive_packet_size = req.buf_cursor;
		  req.status = ARCHIVING_PAGE;
	       } else {
		  // done
		  req.status = DONE;
		  status = 0;
	       }
	       break;
	    } else if (bytes_written >= 0) {
	       archive_packet_cursor += bytes_written;
	       break;
	    } else {
	       // error in archiving
	       LOG(("Error: Error in archiving page!!!!!!!"));
	       req.status = DONE;
	       status = RC_FM_CANNOT_WRITE_SOCKET;
	       break;
	    }
	 }
	 default:
	    assert(0);
      }
      if (req.status == DONE) {
          // finish request
	  finish_request(archive_queue[archive_queue_cursor], status);

	  // remove this entry from archive queue
	  archive_packet_cursor++;
	  archive_packet_cursor %= MAX_PENDING_REQUESTS;
	  archive_packet_size--;
      }
   }
   */

   return 0;
}


/*
 * add the request to the "pending request table"
 *
 * return : 0  successful,  < 0 error
 * note   : socket 'sd' should have been opened and properly connected!
 */
int fetch_module::append_request(request& req)
{
   //
   // insert request entry to the table
   //

   // find an empty slot in the request table
   int start = cur_request_id % MAX_PENDING_REQUESTS;
   int i = start;
   do {
      i = (i+1) % MAX_PENDING_REQUESTS;
      if (table[i].request_id == 0)  break;
   } while (i != start);
   assert(table[i].request_id == 0);

   // allocate buffer
   char *buffer = new char[INITIAL_BUFFER_SIZE];
   if (buffer == NULL) {
      return RC_FM_CANNOT_ALLOCATE_MEMORY;
   }

   // create http request
   {
      register const char* src;
      register char* bp = buffer;
      char         * end = bp + INITIAL_BUFFER_SIZE;
      register char  c;

      strcpy(bp, "GET ");
      bp += strlen("GET ");

      // copy path
      src = req.page_url->path;
      if (src == NULL || *src == 0) {
	 if (bp < end) *bp++ = '/';
      } else {
	 while (bp < end && (c = *src++) != 0) *bp++ = c;
      }

      // copy parameter
      src = req.page_url->param;
      if (src != NULL && *src != 0) {
	 if (bp < end) {
	    *bp++ = ';';
	    while (bp < end && (c = *src++) != 0) *bp++ = c;
	 }
      }

      // copy query
      src = req.page_url->query;
      if (src != NULL && *src != 0) {
	 if (bp < end) {
	    *bp++ = '?';
	    while (bp < end && (c = *src++) != 0) *bp++ = c;
	 }
      }

      // copy the remainder
      if (snprintf(bp, end - bp, 
		   " HTTP/1.0\r\n"
		   "Host: %s\r\n"
		   "User-Agent: Pita (webmaster@pita.stanford.edu)\r\n"
		   "From: webmaster@pita.stanford.edu\r\n\r\n",
		   req.page_url->net_loc) < 0) {
	 delete [] buffer;
	 return RC_FM_CANNOT_ALLOCATE_MEMORY;
      }
   }

   // connect to the server
   int      sd;
   net_addr serv_addr;

   serv_addr.ip   = req.server_ip;
   serv_addr.port = htons(80);
   if (req.page_url->port != NULL && req.page_url->port[0] != 0) {
      serv_addr.port = htons(atoi(req.page_url->port));
   }
   sd = nm->connect(serv_addr);
   if (sd < 0) {
      delete [] buffer;
      return sd;
   }

   // record request information
   cur_request_id += 
      (i > start) ? (i - start) : (i - start + MAX_PENDING_REQUESTS);
   if (cur_request_id < 0) {
      cur_request_id = i + MAX_PENDING_REQUESTS;
   }

   ++num_pending_requests;
   table[i].request_id = cur_request_id;
   table[i].status     = CONNECTING_TO_SERVER;
   table[i].sd         = sd;
   table[i].callback   = req.callback;
   table[i].callback_param = req.callback_param;
   table[i].buffer     = buffer;
   table[i].buf_cursor = 0;
   table[i].buf_size   = INITIAL_BUFFER_SIZE;
   table[i].start_time = nm->get_time();
   table[i].url        = *req.page_url;

   return cur_request_id;
}


/*
 * delete the request from "pending request table"
 * 
 * return: 0 success,  < 0 error
 * note:   socket 'sd' should have NOT been closed! This function close it!
 */

int fetch_module::finish_request(int entry_num, int return_code)
{
   table_entry& entry = table[entry_num];

   // call the callback function
   response   rsp;
   rsp.request_id    = entry.request_id;
   rsp.error_code    = return_code;
   rsp.page_url      = &entry.url;
   rsp.crawled_time  = time(NULL);
   rsp.response_time = nm->time_diff(nm->get_time(), entry.start_time);
   rsp.page          = entry.buffer;
   rsp.page_size     = entry.buf_cursor;
   entry.buffer[entry.buf_cursor] = 0;
   (*entry.callback)(entry.callback_param, &rsp);
   entry.status = DONE;

   // release the resources allocated
   release_resources(entry_num);
   entry.request_id = 0;
		  
   // decrease pending request count
   num_pending_requests--;

   return 0;
}


int fetch_module::release_resources(int entry_num)
{
   table_entry& entry = table[entry_num];
   assert(entry.request_id != 0);

   if (entry.sd >= 0) {
      nm->close(entry.sd);
      entry.sd = -1;
   }
   if (entry.buffer != NULL) {
      delete [] entry.buffer;
      entry.buffer = NULL;
   }
   return 0;
}
   

/*
 * send the pending request to the server
 * return: 0 successful,  < 0 error
 */

int fetch_module::send_packet(int entry_num)
{
   int  bytes_written, bytes_to_write;
   table_entry &entry = table[entry_num];

   // send the packet
   bytes_to_write = strlen(entry.buffer) - entry.buf_cursor;
   bytes_written = nm->write(entry.sd, entry.buffer + entry.buf_cursor, 
			     bytes_to_write);
   if (bytes_written == bytes_to_write) {
      return 0;
   } else if (bytes_written >= 0) {
      entry.buf_cursor += bytes_written;
      return RC_FM_BEING_TRANSFERRED;
   } else {
       LOG(("Broken Pipe: bytes remaining %d", bytes_to_write));
       return RC_FM_CANNOT_WRITE_SOCKET;
   }

   return 0;
}


/*
 * read response from the server
 * return: 0 successful,  < 0 error
 */

int fetch_module::read_packet(int entry_num)
{
   int  buf_space, bytes_read;
   table_entry &entry = table[entry_num];
	 
   // check the space of the buffer
   buf_space = entry.buf_size - entry.buf_cursor - 1;
   if (buf_space < 1023 && entry.buf_size < MAX_PAGE_SIZE) {
      if (double_buffer_size(entry_num) < 0) {
	 // cannot allocate memory
	 // finish current request
	 return RC_FM_CANNOT_ALLOCATE_MEMORY;
      }
      buf_space = entry.buf_size - entry.buf_cursor - 1;
   }

   // read response
   bytes_read = nm->read(entry.sd, entry.buffer + entry.buf_cursor, buf_space);
   if (bytes_read == net_mod::eof) {
      return 0;
   } else if (bytes_read >= 0) {
      entry.buf_cursor += bytes_read;
      if (entry.buf_size >= MAX_PAGE_SIZE &&
	  entry.buf_cursor >= entry.buf_size) {
	 return RC_FM_TOO_LONG_RESPONSE;
      }
      return RC_FM_BEING_TRANSFERRED;
   } else {
       LOG(("FM: So far %d bytes read", entry.buf_cursor));
      return RC_FM_CANNOT_READ_SOCKET;
   }

   return 0;
}

int fetch_module::double_buffer_size(int entry_num)
{
   table_entry &entry = table[entry_num];

   // double the size of the buffer
   assert(entry.buffer != NULL);
   char* temp =  new char[entry.buf_size *= 2];
   if (temp == NULL) {
      return -1;
   }
   memcpy(temp, entry.buffer, entry.buf_cursor);
   if (entry.buffer != NULL) {
      delete [] entry.buffer;
   }
   entry.buffer = temp;

   return 0;
}
