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
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <string>
#include <vector>

#define BACKLOG 10     /* how many pending connections queue will hold */
#define MAX_RESPONSE_LENGTH 2048 /* maximum length of response */

static void to_lower(string& s);
static int process_request(const string& request, int sockd);

int main(int argc, char* argv[])
{
   int sockfd, new_fd;  /* listen on sock_fd, new connection on new_fd */
   struct sockaddr_in my_addr;    /* my address information */
   struct sockaddr_in their_addr; /* connector's address information */
   unsigned sin_size;
   int opt = 1;

   if (argc != 2) {
      fprintf(stderr, "Usage: %s <port>\n", argv[0]);
      exit(1);
   }

   /* create a socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      perror("socket");
      exit(1);
   }
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
                  (char*)&opt, sizeof(opt)) < 0) {
      perror("setsockopt");
      exit(1);
   }

   /* bind the socket to a port */
   my_addr.sin_family = AF_INET;         /* host byte order */
   my_addr.sin_port = htons(atoi(argv[1]));  /* short, network byte order */
   my_addr.sin_addr.s_addr = INADDR_ANY; /* auto-fill with my IP */
   bzero(&(my_addr.sin_zero), 8);        /* zero the rest of the struct */
   if (bind(sockfd, (struct sockaddr *)&my_addr,
            sizeof(struct sockaddr)) == -1) {
      perror("bind");
      exit(1);
   }

   /* listen */
   if (listen(sockfd, BACKLOG) == -1) {
      perror("listen");
      exit(1);
   }


   /* main loop */
   while(1) {  /* main accept() loop */
      sin_size = sizeof(struct sockaddr_in);
      if ((new_fd = accept(sockfd, (struct sockaddr *)&their_addr, \
                           &sin_size)) == -1) {
         perror("accept");
         continue;
      }
      if (!fork()) { /* this is the child process */
         char buffer[1024];  
	 string request;
	 int bytesread;
         while ((bytesread = read(new_fd, buffer, 1024)) > 0) {
	    string::size_type tmp;
	    request.append(buffer, bytesread);
	    if ((tmp = request.find("\r\n")) != string::npos) {
	       request.erase(tmp);
	       break;
	    }
         }
	 process_request(request, new_fd);
         close(new_fd);
         exit(1);
      }
      close(new_fd);  /* parent doesn't need this */

      while(waitpid(-1,NULL,WNOHANG) > 0); /* clean up child processes */
   }
}


static int process_request(const string& request, int sockd)
{
   string  response;

   string  site_name;
   vector<string>  name_list;

   struct hostent *hp;

   string alias;

   // parse the request
   if (strncmp(request.c_str(), "RESOLVE ", 8) != 0) {
      response.append("ERROR\r\n");
      goto request_exit;
   }
   site_name = request.substr(8);
   to_lower(site_name);
   
   // lookup DNS
   if ((hp = gethostbyname(site_name.c_str())) == NULL) {
      response.append("ERROR\r\n");
      goto request_exit;
   }

   response.append("OK\r\n");

   //
   // IP list
   //
   for (char **p = hp->h_addr_list; *p != 0; p++) {
      struct in_addr in;
      memcpy(&in.s_addr, *p, sizeof (in.s_addr));
      if (response.length() + strlen(inet_ntoa(in)) + 7 
	  >= MAX_RESPONSE_LENGTH) goto request_exit;
      response.append("IP ");
      response.append(inet_ntoa(in));
      response.append("\r\n");
   }

   //
   // site name list
   //
   name_list.push_back(site_name);
   alias = hp->h_name;
   to_lower(alias);
   if (alias != site_name) {
      name_list.push_back(alias);
   }

   for (char **q = hp->h_aliases; *q != 0; q++) {
      alias = *q;
      to_lower(alias);
      bool exist = false;
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
   for (int i = 0; i < (int)name_list.size(); i++) {
      if (response.length() + name_list[i].length() + 9 
	  >= MAX_RESPONSE_LENGTH)  goto request_exit;
	 
      response.append("NAME ");
      response.append(name_list[i]);
      response.append("\r\n");
   }

 request_exit:
   response.append("\r\n");
   write(sockd, response.data(), response.length());
   return 0;
}

static void to_lower(string& s)
{
   register char c;
   register int i, len = s.size();
   for (i = 0; i < len; i++) {
      if ('A' <= (c = s[i]) && c <= 'Z') {
	 s[i] = (c + ('a' - 'A'));
      }
   }
}
