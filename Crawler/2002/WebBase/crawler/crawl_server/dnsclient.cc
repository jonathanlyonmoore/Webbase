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
#include <string>
#include <vector>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

int init_dnshelper_client(const string& dnshelper, unsigned short port);
int resolve_name(const string& site_name,  
		 vector<string>& ip_list, vector<string>& name_list);

int main(int argc, char* argv[]) 
{
   vector<string> ip_list;
   vector<string>   name_list;

   init_dnshelper_client(string("pita"), 3000);
   resolve_name(string(argv[1]), ip_list, name_list);
   

   for (unsigned i = 0; i < ip_list.size(); i++) {
      printf("IP '%s'\n", ip_list[i].c_str());
   }
   for (unsigned i = 0; i < name_list.size(); i++) {
      printf("NAME '%s'\n", name_list[i].c_str());
   }

   return 0;
}

static struct sockaddr_in dnsserver_addr;

int init_dnshelper_client(const string& dnshelper, unsigned short port)
{
      /* resolve the host name into ip address */
   if (inet_aton(dnshelper.c_str(), &dnsserver_addr.sin_addr) == 0) {
      struct hostent *he;
      if ((he = gethostbyname(dnshelper.c_str())) == NULL) {
          return -1;
      }
      dnsserver_addr.sin_addr = *((struct in_addr *)he->h_addr);
   }

   dnsserver_addr.sin_family = AF_INET;   /* host byte order */
   dnsserver_addr.sin_port = htons(port); /* network byte order */
// BEGIN SOLARIS PORT CHANGE
   // bzero(&(dnsserver_addr.sin_zero), 8);  /* zero the rest of the struct */
   memset(&(dnsserver_addr.sin_zero), 0, 8);
// END SOLARIS PORT CHANGE
   return 0;
}
      
int resolve_name(const string& site_name,  
		 vector<string>& ip_list, vector<string>& name_list)
{
   int   sockfd, cursor, temp;
   string  request;
   char  buffer[2048];
   char *linestart, *lineend;

   /* create a socket */
   if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      return -1;
   }

   /* connect to the server */
   if (connect(sockfd, (struct sockaddr *)&dnsserver_addr, \
               sizeof(struct sockaddr)) == -1) {
      goto error_exit;
   }

   /* send request */
   request = site_name + "\r\n";
   cursor = 0;
   while (cursor < (int)request.size()) {
      temp = write(sockfd, request.data() + cursor, request.size() - cursor);
      if (temp >= (int)request.size() - cursor)  break;
      if (temp < 0) goto error_exit;
      cursor += temp;
   }
   shutdown(sockfd, 1);

   /* read response */
   cursor = 0;
   while ((temp = read(sockfd, buffer + cursor, 2048 - 1 - cursor)) > 0) {
      cursor += temp;
   }
   if (cursor <= 0) goto error_exit;
   buffer[cursor] = 0;

   /* parse response */
   linestart = buffer;
   while ((lineend = strstr(linestart, "\r\n")) != NULL) {
      *lineend = 0;
      if (strlen(linestart) >= 3 && strncmp(linestart, "IP ", 3) == 0) {
	 static struct sockaddr_in ipaddr;
	 if (inet_aton(linestart + 3, &ipaddr.sin_addr) != 0) {
	    ip_list.push_back(string(linestart+3));
	 }
      } else if (strlen(linestart) >= 5, strncmp(linestart, "NAME ", 5) == 0) {
	 name_list.push_back(linestart+5);
      }
      linestart = lineend + 2;
      if (linestart > buffer+cursor) break;
   }

   if (ip_list.size() <= 0 || name_list.size() <= 0) {
      goto error_exit;
   }
   close(sockfd);
   return 0;

 error_exit:
   close(sockfd);
   return -1;
}
