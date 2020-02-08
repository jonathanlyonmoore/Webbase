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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <iostream>
#include "crawl_comm.h"

crawl_comm::crawl_comm(net_mod* nm, const net_addr& local, 
		       const net_addr& site_server)
   : nm(nm), site_server_addr(site_server), 
     main_sd(-1), request_sd(-1), report_sd(-1), new_site_sd(-1),
     request_status(NONE)
{
   // create a socket to monitor the request from site server
   main_sd = nm->listen(local, 10);
   if (main_sd < 0) {
      cerr << "Cannot bind socket" << endl;
      exit(1);
   }
}

crawl_comm::~crawl_comm()
{
   if (main_sd >= 0) {
      nm->close(main_sd);
   }
   if (request_sd >= 0) {
      nm->close(request_sd);
   }
   if (report_sd >= 0) {
      nm->close(report_sd);
   }
   if (new_site_sd >= 0) {
      nm->close(new_site_sd);
   }
}

int crawl_comm::get_request(string& request)
{   
   if (request_status != REQUEST_READY)  return RC_CC_NO_PENDING_REQUEST;
   
   request = buffer;
   request_status = REQUEST_READ;

   buffer.erase();
   buffer_cursor = 0;

   return 0;
}

int crawl_comm::send_response(const string& response)
{
   if (request_status != REQUEST_READ)  return RC_CC_NO_PENDING_REQUEST;

   buffer = response;
   buffer_cursor = 0;

   nm->set_write(request_sd);
   request_status = SENDING_RESPONSE;

   return 0;
}

int crawl_comm::send_report(const string& report)
{
   this->report.push_back(report);

   if (report_sd <= 0) {
      report_sd = nm->connect(site_server_addr);
      if (report_sd < 0) {
	 return RC_CC_CANNOT_CONTACT_SITE_SERVER;
      }

      report_cursor = 0;
   }

   return 0;
}

int crawl_comm::send_new_site(const string& new_site)
{
   if (this->new_site.size() <= 0) {
      new_site_sd = nm->connect(site_server_addr);
      if (new_site_sd < 0) {
	 return RC_CC_CANNOT_CONTACT_SITE_SERVER;
      }

      new_site_cursor = 0;
   }

   this->new_site.push_back(new_site);
   return 0;
}


int crawl_comm::process()
{
   nm->poll(0);

   //
   // process crawling request from the site server
   //
   switch (request_status) {
      case NONE:
	 if (nm->ready(main_sd)) {
	    if ((request_sd = nm->accept(main_sd)) < 0)  break;

	    buffer.erase();
	    buffer_cursor = 0;
	    request_status = READING_REQUEST;
	 }
	 break;

      case READING_REQUEST:
	 if (nm->ready(request_sd)) {
	    // read command from socket
	    const int bufSize = 1024;
	    char tmp_buf[bufSize];
	    int bytes_read = nm->read(request_sd, tmp_buf, bufSize-1);
	    
	    // check error
	    if (bytes_read != net_mod::eof && bytes_read < 0) {
	       nm->close(request_sd);
	       request_status = NONE;
	       break;
	    }
	    
	    if (bytes_read > 0) {
	       buffer.append(tmp_buf, tmp_buf + bytes_read);
	       buffer_cursor += bytes_read;
	    }

	    // check whether we the request ended
	    unsigned request_end;

            // FIXME gcc warning: if bytes_read==eof, request_end not init'd
            // before use inside this if() clause. 
	    if (bytes_read == net_mod::eof ||
		(request_end = buffer.find("\r\n\r\n")) != string::npos) {
	       //
	       // request ended
	       //
	       if (request_end != string::npos) {
		  buffer_cursor = request_end + 2;
		  buffer.erase(buffer_cursor);
	       }
	       
	       nm->suspend(request_sd);
	       request_status = REQUEST_READY;
	    }
	 }
	 break;

      case REQUEST_READY:
	 break;
	 
      case SENDING_RESPONSE:
	 if (nm->ready(request_sd)) {
	    int bytes_to_write, bytes_written;
	    bytes_to_write = buffer.length() - buffer_cursor;
	    bytes_written = nm->write(request_sd, 
				      buffer.data() + buffer_cursor,
				      bytes_to_write);
	    if (bytes_written == bytes_to_write || bytes_written < 0) {
	       nm->close(request_sd);

	       buffer.erase();
	       buffer_cursor = 0;
	       request_status = NONE;
	    } else if (bytes_written >= 0) {
	       buffer_cursor += bytes_written;
	    }
	 }
	 break;
   }

   //
   // send crawling report
   //
   if (report.size() > 0) {
    if (report_sd < 0) {
      report_sd = nm->connect(site_server_addr);
      if (report_sd < 0)  report_sd = -1;
    }
    if (report_sd >= 0 && nm->ready(report_sd)) {
      int bytes_to_write, bytes_written;

      bytes_to_write = report[0].size() - report_cursor;
      bytes_written = nm->write(report_sd, report[0].data() + report_cursor,
				bytes_to_write);
      if (bytes_written == bytes_to_write || bytes_written < 0) {
	 nm->close(report_sd);
	 report_sd = -1;
	 if (bytes_written == bytes_to_write) report.erase(report.begin());
	 report_cursor = 0;
	 
	 if (report.size() > 0) {
	    report_sd = nm->connect(site_server_addr);
	    if (report_sd < 0)  report_sd = -1;
	 }
      } else if (bytes_written >= 0) {
	 report_cursor += bytes_written;
      }
    }
   }
	 
   //
   // send new sites found
   //
   if (new_site.size() > 0 && nm->ready(new_site_sd)) {
      int bytes_to_write, bytes_written;

      bytes_to_write = new_site[0].size() - new_site_cursor;
      bytes_written = nm->write(new_site_sd, 
				new_site[0].data() + new_site_cursor,
				bytes_to_write);
      if (bytes_written == bytes_to_write || bytes_written < 0) {
	 nm->close(new_site_sd);
	 new_site_sd = -1;
	 new_site.erase(new_site.begin());
	 new_site_cursor = 0;
	 
	 if (new_site.size() > 0) {
	    new_site_sd = nm->connect(site_server_addr);
	    if (new_site_sd < 0) {
	       new_site.erase(new_site.begin(), new_site.end());
	       new_site_sd = -1;
	    }
	 }
      } else if (bytes_written >= 0) {
	 new_site_cursor += bytes_written;
      }
   }

   return 0;
}
 
