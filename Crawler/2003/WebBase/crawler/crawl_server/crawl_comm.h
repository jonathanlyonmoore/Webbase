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
#ifndef CRAWL_COMM_H
#define CRAWL_COMM_H

#include <string>
#include <vector>
#include "net_mod.h"

const int RC_CC_ERROR_START = -700;
const int RC_CC_NO_PENDING_REQUEST          = RC_CC_ERROR_START - 1;
const int RC_CC_RESPONSE_TOO_LONG           = RC_CC_ERROR_START - 2;
const int RC_CC_CANNOT_CONTACT_SITE_SERVER  = RC_CC_ERROR_START - 3;
/*
const int RC_CC_                            = RC_CC_ERROR_START - 4;
const int RC_CC_                            = RC_CC_ERROR_START - 5;
*/
class crawl_comm {
   public:
      crawl_comm(net_mod* nm, const net_addr& local_addr, 
		 const net_addr& site_server_addr);
      ~crawl_comm();

      int  get_request(std::string& request);
      int  send_response(const std::string& response);
      int  send_report(const std::string& report);
      int  send_new_site(const std::string& sites);

      int  process();

      // FIXME Who uses this constant?  If anybody, does it need adjustment?
      // static const int MAX_REQUEST_SIZE = 10*1024;

   private:
      net_mod    *nm;

      net_addr   site_server_addr;

      int   main_sd;
      int   request_sd;
      int   report_sd;
      int   new_site_sd;

      int   request_status;
      static const int NONE = 0;
      static const int READING_REQUEST = 1;
      static const int REQUEST_READY   = 2;
      static const int REQUEST_READ    = 3;
      static const int SENDING_RESPONSE = 4;

      std::string buffer;
      int    buffer_cursor;
      
      std::vector<std::string> report;
      int    report_cursor;

      std::vector<std::string> new_site;
      int    new_site_cursor;
};
      
#endif // CRAWL_COMM_H
