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
#ifndef NET_MOD_H
#define NET_MOD_H

#include <sys/time.h>
#include <vector>
#include <string>

const int RC_CN_ERROR_START           = -600;
const int RC_CN_TOO_MANY_ENTRIES      = RC_CN_ERROR_START - 1;
const int RC_CN_INVALID_PARAMETER     = RC_CN_ERROR_START - 2;
const int RC_CN_SELECT_ERROR          = RC_CN_ERROR_START - 3;
const int RC_CN_CANNOT_CREATE_SOCKET  = RC_CN_ERROR_START - 4;
const int RC_CN_SOCKET_ERROR          = RC_CN_ERROR_START - 5;
const int RC_CN_CANNOT_SET_SOCKETFLAG = RC_CN_ERROR_START - 6;
const int RC_CN_CANNOT_BIND_SOCKET    = RC_CN_ERROR_START - 7;
const int RC_CN_CANNOT_LISTEN_SOCKET  = RC_CN_ERROR_START - 8;
const int RC_CN_CANNOT_ACCEPT         = RC_CN_ERROR_START - 9;
const int RC_CN_SOCKET_CLOSE_ERROR    = RC_CN_ERROR_START - 10;
const int RC_CN_CANNOT_CONNECT_TO_SERVER = RC_CN_ERROR_START - 11;
const int RC_CN_NO_PENDING_REQUEST       = RC_CN_ERROR_START - 12;
const int RC_CN_CANNOT_READ_SOCKET       = RC_CN_ERROR_START - 13;
const int RC_CN_CANNOT_WRITE_SOCKET      = RC_CN_ERROR_START - 14;

struct net_addr {
      unsigned ip;
      short    port;
};

class net_mod {
   public:
      static const int NONBLOCKING = 0;
      static const int BLOCKING    = 1;
      net_mod(int type = NONBLOCKING);

      int  set_read(int sd);
      int  set_write(int sd);
      int  suspend(int sd);
      int  poll(int wait);
      int  poll(int sd, int wait);

      int listen(const net_addr& addr, int max_pending = 5);
      int accept(int sd);
      int connect(const net_addr& addr);

      static const int READ = 0;
      static const int WRITE = 1;
      static const int BOTH  = 2;

      bool ready(int sd);
      int  check_error(int sd);
      int  close(int sd, int direction = BOTH);

      static const int eof = -1;

      int read(int sd, char* start, int size);
      int write(int sd, const char* start, int size);

      static int resolve_name(const std::string& site_name, 
			      std::vector<unsigned>& ip_list, 
			      std::vector<std::string>& name_list);
      static unsigned get_hostip();

      static unsigned inet_addr(const std::string& ip);
      static const std::string inet_ntoa(unsigned ip);

      static int get_time(int diff = 0);
      static int time_diff(int time1, int time2);
      static int time_add(int time, int diff);

      static const int MAX_NUM_SOCKETS = 70;

   private:
      static int set_nonblocking(int sd);

      bool   blocking;
      int    max_socket;
      
      fd_set readfds;
      fd_set writefds;

      fd_set readfds_ready;
      fd_set writefds_ready;

};

int finish_process();

#endif




