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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <sys/times.h>
#include <ctime>
#include <cerrno>
#include <string>
#include <cassert>
#include "my_utility.h"
#include "net_mod.h"

using std::string;
using std::vector;
// Has CLK_TCK been superseded?
#ifndef CLK_TCK
#define CLK_TCK (CLOCKS_PER_SEC)
#endif

net_mod::net_mod(int type)
   : max_socket(-1)
{
   FD_ZERO(&readfds);
   FD_ZERO(&writefds);
   FD_ZERO(&readfds_ready);
   FD_ZERO(&writefds_ready);
   
   blocking = (type == BLOCKING);
}



//
// function: set the socket in "read" mode
// input: sd - socket descriptor
// output: 0
// error:  none

int net_mod::set_read(int sd)
{
   assert(sd >= 0);

   FD_SET(sd, &readfds);
   FD_CLR(sd, &writefds);
   if (max_socket < sd)  max_socket = sd;

   return 0;
}

//
// function: set the socket in "write" mode
// input: sd - socket descriptor
// output: 0
// error:  none

int net_mod::set_write(int sd)
{
   assert(sd >= 0);

   FD_SET(sd, &writefds);
   FD_CLR(sd, &readfds);
   if (max_socket < sd)  max_socket = sd;

   return 0;
}

//
// function: block read/write from the socket
// input: sd - socket descriptor
// output: 0
// error: none

int net_mod::suspend(int sd)
{
   assert(sd >= 0);

   FD_CLR(sd, &readfds);
   FD_CLR(sd, &writefds);

   // adjust max_socket
   if (max_socket == sd) {
      for (int i = max_socket; i >= 0; i--) {
	 if (FD_ISSET(i, &readfds) || FD_ISSET(i, &writefds)) {
	    max_socket = i;
	    return 0;
	 }
      }
      max_socket = -1;
   }
   return 0;
}

//
// function: poll over the open sockets
// input: wait_until - expiration time (type : return from get_time())
// output: number of sockets ready
// error: no error condition
//

int net_mod::poll(int wait_until)
{
   int    num_ready;

   // set up timer
   struct timeval tv;
   struct timeval *tvp = (wait_until < 0) ? NULL : &tv;

 select_loop:
   tv.tv_sec = tv.tv_usec = 0;
   if (wait_until > 0) {
      int now = get_time();
      if (now < wait_until) {
	 int diff = time_diff(wait_until, now);
	 if (diff > 0) {
	    tv.tv_usec = diff % 1000;
	    tv.tv_sec  = diff / 1000;
	 }
      }
   }

   memcpy(&readfds_ready,  &readfds, sizeof(readfds));
   memcpy(&writefds_ready, &writefds, sizeof(writefds));

   // poll the sockets
   num_ready = select(max_socket+1, &readfds_ready, &writefds_ready, 
		      NULL, tvp);
   if (num_ready < 0) {
      if (errno == EINTR) goto select_loop;
      assert(errno != EINVAL);
      assert(errno != EBADF);
      
      // some weird error
      LOG(("Unexpected error: %s(%d)", strerror(errno), errno));
      //finish_process();
   }
   
   return num_ready;
}


//
// function: poll over the socket
// input: wait_until - expiration time (type : return from get_time())
// output: 1 if ready, 0 if not
// error: no error condition
//

int net_mod::poll(int sd, int wait_until)
{
   int    num_ready;

   // set up timer
   struct timeval tv;
   struct timeval *tvp = (wait_until < 0) ? NULL : &tv;

   fd_set fds;
   fd_set *readfdp, *writefdp;

 select_loop:
   FD_ZERO(&fds);
   FD_SET(sd, &fds);
   readfdp = writefdp = NULL;
   if (FD_ISSET(sd, &readfds)) {
      readfdp = &fds;
   } else if (FD_ISSET(sd, &writefds)) {
      writefdp = &fds;
   }

   tv.tv_sec = tv.tv_usec = 0;
   if (wait_until > 0) {
      int now = get_time();
      if (now < wait_until) {
	 int diff = time_diff(wait_until, now);
	 if (diff > 0) {
	    tv.tv_usec = diff % 1000;
	    tv.tv_sec  = diff / 1000;
	 }
      }
   }

   // poll the sockets
   num_ready = select(sd+1, readfdp, writefdp, NULL, tvp);
   if (num_ready < 0) {
      if (errno == EINTR) goto select_loop;
      assert(errno != EINVAL);
      assert(errno != EBADF);
      
      // some weird error
      LOG(("Unexpected error: %s(%d)", strerror(errno), errno));
      //finish_process();
   }
   
   return num_ready;
}


// function : check whether the socket is ready to read/write
// input:  sd - socket descriptor
// output: true/false
// error:  none

bool net_mod::ready(int sd)
{
   assert(sd >= 0);
   poll(0);
   return (FD_ISSET(sd, &readfds_ready) || FD_ISSET(sd, &writefds_ready));
}

//
// function : check error condition of the socket
// input: sd - socket descriptor
// output : errno
// error: none
//
int net_mod::check_error(int sd)
{
#if 0 /* obsolete */
#if defined(LINUX)
   unsigned  err_code, dummy = sizeof(int);
#else
   int       err_code, dummy = sizeof(int);
#endif
#else
   int       err_code;
   socklen_t dummy = sizeof(int);
#endif

   if (getsockopt(sd, SOL_SOCKET, SO_ERROR, (char*)&err_code, &dummy) < 0) {
      assert(errno != EBADF);
      assert(errno != ENOPROTOOPT);
      assert(errno != ENOTSOCK);
      
      err_code = errno;
   }

   return err_code;
}

// function: close the socket
// input: sd - socket descriptor, direction : READ/WRITE
// output: 0 / RC_CN_SOCKET_CLOSE_ERROR
// error: RC_CN_SOCKET_CLOSE_ERROR

int net_mod::close(int sd, int direction)
{
   int  rc;
 close_loop:
   if (direction == BOTH) {
      suspend(sd);
      rc = ::close(sd);
      if (rc < 0) {
	 if (errno == EINTR) goto close_loop;
	 assert(errno != EBADF);
	 return RC_CN_SOCKET_CLOSE_ERROR;
      }
   } else {
      if (direction == READ) {
	 FD_CLR(sd, &readfds);
      } else {
	 FD_CLR(sd, &writefds);
      }
      rc = ::shutdown(sd, direction);
      if (rc < 0) {
	 assert(errno != EBADF);
	 assert(errno != ENOTSOCK);
	 return RC_CN_SOCKET_CLOSE_ERROR;
      }
   }

   return 0;
}

// function: listen on the specified address
// input: addr, max_pending - maximum pending requests
// output: 0 / error
// error: RC_CN_CANNOT_CREATE_SOCKET
//        RC_CN_CANNOT_SET_SOCKETFLAG
//        RC_CN_CANNOT_BIND_SOCKET
//        RC_CN_CANNOT_LISTEN_SOCKET
int net_mod::listen(const net_addr& addr, int max_pending)
{
   int  sd;
   int  opt = 1;

   // create socket
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      return RC_CN_CANNOT_CREATE_SOCKET;
   }
   
   // set the socket "NON-blocking"
   if (!blocking) {
      if (set_nonblocking(sd) < 0) {
	 ::close(sd);
	 return RC_CN_CANNOT_SET_SOCKETFLAG;
      }
   }
  
   if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt))<0) {
      return RC_CN_CANNOT_SET_SOCKETFLAG;
   }

   // bind the socket
   struct sockaddr_in my_addr;
   my_addr.sin_family      = AF_INET;
   my_addr.sin_addr.s_addr = addr.ip;
   my_addr.sin_port        = addr.port;
   memset(&(my_addr.sin_zero), 0, 8);
   if (bind(sd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0) {
      ::close(sd);
      return RC_CN_CANNOT_BIND_SOCKET;
   }
   
   // listen
   if (::listen(sd, max_pending) < 0) {
      ::close(sd);
      return RC_CN_CANNOT_LISTEN_SOCKET;
   }

   set_read(sd);

   return sd;
}

// function: accept request
// input: poll_sd - socket to get the request
// output: new socket created/ error
// error: RC_CN_CANNOT_CREATE_SOCKET
//        RC_CN_NO_PENDING_REQUEST
//        RC_CN_CANNOT_ACCEPT
//        RC_CN_CANNOT_SET_SOCKETFLAG

int net_mod::accept(int poll_sd)
{
   int      sd;
   struct   sockaddr_in client_addr;
#if 0 /* obsolete */
#if defined(LINUX)
   unsigned addrlen;
#else
   int      addrlen;
#endif
#else
   socklen_t addrlen;
#endif

 accept_loop:
   addrlen = sizeof(client_addr);
   sd = ::accept(poll_sd, (sockaddr*)&client_addr, &addrlen);
   if (sd < 0) {
      if (errno == EINTR)  goto accept_loop;
      if (errno == EMFILE) return RC_CN_CANNOT_CREATE_SOCKET;
      if (errno == EWOULDBLOCK) return RC_CN_NO_PENDING_REQUEST;

      return RC_CN_CANNOT_ACCEPT;
   }
   
   // set the socket "NON-blocking"
   if (!blocking) {
      if (set_nonblocking(sd) < 0) {
	 ::close(sd);
	 return RC_CN_CANNOT_SET_SOCKETFLAG;
      }
   }

   set_read(sd);

   return sd;
}

// function: connect to the remote server
// input: addr - remote address
// output: socket descriptor
// error: RC_CN_CANNOT_CREATE_SOCKET
//        RC_CN_CANNOT_SET_SOCKETFLAG
//        RC_CN_CANNOT_CONNECT_TO_SERVER

int net_mod::connect(const net_addr& addr)
{
   int    sd;
   struct sockaddr_in serv_addr;  

   // create a socket
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
     return RC_CN_CANNOT_CREATE_SOCKET;
   }
   
   // set the socket "NON-blocking"
   if (!blocking) {
      if (set_nonblocking(sd)) {
	 ::close(sd);
	 return RC_CN_CANNOT_SET_SOCKETFLAG;
      }
   }
  
 connect_loop:
   // connect to server
   memset((char*) &serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family      = AF_INET;   // internet address family
   serv_addr.sin_addr.s_addr = addr.ip;   // ip should be in network order
   serv_addr.sin_port        = addr.port; // port should be in network order
   if(::connect(sd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
      if (errno == EINTR) goto connect_loop;
      assert(errno != EBADF);
      assert(errno != ENOTSOCK);
      assert(errno != EISCONN);
      assert(errno != EALREADY);
      if (errno != EINPROGRESS) {
	 ::close(sd);
	 return RC_CN_CANNOT_CONNECT_TO_SERVER;
      }
   }

   set_write(sd);

   return sd;
}

inline int net_mod::set_nonblocking(int sd)
{
   return fcntl(sd, F_SETFL, O_NONBLOCK);
}

// function : read "size" bytes from socket "sd" to "start"
// input  : sd, start, size
// return : # of bytes read, 
//          net_mod::eof  - when the other side closed socket
// error: RC_CN_CANNOT_READ_SOCKET

int net_mod::read(int sd, char* start, int size)
{
   assert(size <= SSIZE_MAX);
   assert(FD_ISSET(sd, &readfds));
   assert(sd <= max_socket);

   // check the status of the socket
#if !defined(LINUX)
   unsigned  err_code = 0, dummy = sizeof(int);

   getsockopt(sd, SOL_SOCKET, SO_ERROR, (char*)&err_code, &dummy);
   if (err_code != 0) {
      LOG(("SocketError: read: %s", strerror(err_code)));
      return RC_CN_CANNOT_READ_SOCKET;
   }
#endif // LINUX

   int bytes_read = ::read(sd, start, size);
   if (bytes_read == 0) return net_mod::eof;
   else if (bytes_read < 0) {
      if (errno == EAGAIN) return 0;
      else {
	LOG(("SocketError: read: %s", strerror(errno)));
	return RC_CN_CANNOT_READ_SOCKET;
      }
   }
   
   return bytes_read;
}

// function : read "size" bytes from to "start" socket "sd"
// input  : sd, start, size
// return : # of bytes written, 
// error:   RC_CN_CANNOT_WRITE_SOCKET

int net_mod::write(int sd, const char* start, int bytes_to_write)
{
   assert(FD_ISSET(sd, &writefds));
   assert(sd <= max_socket);
   int  bytes_written;

   // check the status of the socket
#if !defined(LINUX)
#if 0 /* obsolete */
   int       err_code = 0, dummy = sizeof(int);
#else
   int       err_code = 0;
   socklen_t  dummy = sizeof(int);
#endif
   getsockopt(sd, SOL_SOCKET, SO_ERROR, (char*)&err_code, &dummy);
   if (err_code != 0)  return RC_CN_CANNOT_WRITE_SOCKET;
#endif

   // send the request
   bytes_written = ::write(sd, start, bytes_to_write);
   if (bytes_written < 0) {
      if (errno == EAGAIN || errno == EINTR) return 0;

      LOG(("SocketError: write: %s", strerror(errno)));
      return RC_CN_CANNOT_WRITE_SOCKET;
   }

   return bytes_written;
}


int net_mod::resolve_name(const std::string& site_name, 
			  std::vector<unsigned>& ip_list, 
			  std::vector<std::string>& name_list)
{
   struct hostent hst, *hp;
   const int BufferSize = 2*1024;
   char   buffer[BufferSize];

   string alias;

   ip_list.erase(ip_list.begin(), ip_list.end());
   name_list.erase(name_list.begin(), name_list.end());

   if (site_name.size() > 256)  return -1;

   // lookup DNS
// #if defined(LINUX)
// An autoconf-assisted OS-independent approach:
#if GETHOSTBYNAME_R_ARGS==6
   int    herrno;
   if (gethostbyname_r(site_name.c_str(), 
		       &hst, buffer, BufferSize, &hp, &herrno) < 0) {
      return -1;
   }
#elif GETHOSTBYNAME_R_ARGS==5
   hp = gethostbyname_r(site_name.c_str(), &hst, buffer, BufferSize, &h_errno);
   if (hp == NULL) {
      return -1;
   }
#else
#error gethostbyname_r type not recognized; please fill in code.
#endif

   //
   // store mapped IPs
   //
   for (char **p = hp->h_addr_list; *p != 0; p++) {
      struct in_addr in;
      memcpy(&in.s_addr, *p, sizeof (in.s_addr));
      ip_list.push_back(in.s_addr);
   }

   //
   // store mapped site names
   //
   name_list.push_back(site_name);

   alias = hp->h_name;
   to_lower(alias);
   if (alias != site_name) {
      name_list.push_back(alias);
   }

   bool exist;
   for (char **q = hp->h_aliases; *q != 0; q++) {
      alias = *q;
      exist = false;
      to_lower(alias);
      for (int i = 0; (unsigned)i < name_list.size(); i++) {
	 if (name_list[i].compare(alias) == 0) {
	    exist = true;
	    break;
	 }
      }
      if (!exist) {
	 name_list.push_back(alias);
      }
   }
   
   return 0;
}


unsigned net_mod::get_hostip()
{
   unsigned ip;
   char     hostname[MAXHOSTNAMELEN];

   if (gethostname(hostname, MAXHOSTNAMELEN) < 0) {
      return 0;
   }
   struct hostent *hp = gethostbyname(hostname);
   if (hp == NULL) {
      return 0;
   }
   memcpy(&ip, hp->h_addr, 4);

   return ip;
}

unsigned net_mod::inet_addr(const std::string& ip_str)
{
   union {
	 unsigned char digit[4];
	 unsigned all;
   } ip;
   
   vector<string> digits;
   split_string(ip_str, ".", digits);
   if (digits.size() != 4)  return (unsigned)-1;
   
   for (int i = 0; i < (int)digits.size(); i++) {
      int digit = atoi(digits[i].c_str());
      if (digit < 0 || digit >= 512)  return (unsigned)-1;
      ip.digit[i] = digit;
   }
   
   return ip.all;
}

const std::string net_mod::inet_ntoa(unsigned my_ip)
{
   string result;
   union {
	 unsigned char digit[4];
	 unsigned all;
   } ip;

   ip.all = my_ip;
   result = itoa(ip.digit[0]);
   for (int i = 1; i < 4; i++) {
      result += ".";
      result += itoa(ip.digit[i]);
   }

   return result;
}
   
   
int net_mod::get_time(int diff)
{
   struct tms  dummy;

   return (diff == 0) ? times(&dummy) : (times(&dummy) + diff*CLK_TCK/1000);
}

int net_mod::time_diff(int time1, int time2)
{
   return (int)(((long long)(time1 - time2))*1000/CLK_TCK);
}

int net_mod::time_add(int time, int diff)
{
   return time + diff*CLK_TCK/1000;
}

