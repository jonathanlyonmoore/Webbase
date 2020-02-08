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
#include <stdio.h>
#include <iostream.h>
#include <fstream.h>
#include <sys/types.h>
#include <netinet/in.h>
#ifdef SOCKET_CONTROL
#include <unistd.h>
#endif
#include "url_filter.h"
#include "manager.h"
#include "my_utility.h"
#include "crawl_utility.h"

// BEGIN SOLARIS PORT CHANGE
#include "../../port/include/daemon.h"
// BEGIN SOLARIS PORT CHANGE

//Pranav start
#include "parameter.h"
//Pranav end

int  CreateLog(const char* name, int maxline);

#if defined(HISTORY)
#define HISTORYFILENAME "history/history"
FILE* _HISTORYFILE;
#endif

vector<string>* disklist;
char repName[1024];

int main(int argc, const char* argv[])
{
  //Pranav start
  Parameter crawlparam(PARAMETER_FILE);
  vector<string> crawl_list;
  //Pranav end

   int  crawler_id = -1;

   net_addr local_addr;
   net_addr site_addr;
   net_addr archiver_addr;

   //Pranav start
   string strLocal, strSite, strConfig;
   string strExplanation;
   string strExtensionsParse, strExtensionsStore;
   string strFollowIMG;
   string logFileName("log/crawl_server");
   vector<string> str_list;

   strExplanation = "Local Port";
   //pass blank option, which means this cannot be a command line option
   crawlparam.CreateRecord(&strLocal, CRAWLSERVER, "LocalAddress", "l", strExplanation);

   strExplanation = "Site Server Port";
   crawlparam.CreateRecord(&strSite, SITESERVER, "LocalAddress", "s", strExplanation);

   strExplanation = "Configuration file";
   crawlparam.CreateRecord(&strConfig, CRAWLSERVER, "ConfigFile", "c", strExplanation);

   strExplanation = "Crawler ID";
   //pass a blank tag, which means entry doesn't exist in config file
   crawlparam.CreateRecord(&crawler_id, CRAWLSERVER, "", "id", strExplanation);

   strExplanation = "LogFile ";
   crawlparam.CreateRecord(&logFileName, CRAWLSERVER, "", "LogFile", strExplanation);

#ifdef SOCKET_CONTROL
   int daemonize = 0;
   strExplanation = "detach as daemon";
   crawlparam.CreateRecord(&daemonize, CRAWLSERVER, "", "daemon", strExplanation);
#endif

   strExplanation = "Extensions (follow links)";
   crawlparam.CreateRecord(&strExtensionsParse, CRAWLSERVER, "ExtParse", "p", strExplanation);

   strExplanation = "Extensions (store)";
   crawlparam.CreateRecord(&strExtensionsStore, CRAWLSERVER, "ExtStore", "t", strExplanation);

   strExplanation = "Follow <img> links";
   crawlparam.CreateRecord(&strFollowIMG, CRAWLSERVER, "FollowIMG", "i", strExplanation);

   // Process any command line options
   crawlparam.ProcessCommandLineOptions(argc, argv);

   if (crawler_id < 0) {
     cerr << "Invalid Crawler ID\n";
     exit(1);
   }

   split_string(strExtensionsParse, ",", str_list);
   url_filter::set_parse_list(str_list);

   split_string(strExtensionsStore, ",", str_list);
   url_filter::set_store_list(str_list);

   url_filter::set_img_enabled(strFollowIMG == "1");


   // Jun 2002
   /*
   split_string(strLocal, ",", str_list);
   strLocal = str_list[crawler_id];
   */

   //cout << "local address: " << strLocal << endl; //debug

   split_string(strLocal, ":", str_list);
   local_addr.ip   = net_mod::inet_addr(str_list[0]);
   if (local_addr.ip == (unsigned)-1) {
      vector<string> name;
      vector<unsigned> ip;

      if (net_mod::resolve_name(str_list[0], ip, name) < 0) {
	 cerr << "Invalid local address" << endl;
	 exit(1);
      }
      local_addr.ip = ip[0];
   }
   local_addr.port = htons(atoi(str_list[1].c_str()));

   //cout << "siteserver address: " << strSite << endl; //debug

   split_string(strSite, ":", str_list);
   site_addr.ip   = net_mod::inet_addr(str_list[0]);
   if (site_addr.ip == (unsigned)-1) {
      vector<string> name;
      vector<unsigned> ip;

      if (net_mod::resolve_name(str_list[0], ip, name) < 0) {
	 cerr << "Invalid local address" << endl;
	 exit(1);
      }
      site_addr.ip = ip[0];
   }
   site_addr.port = htons(atoi(str_list[1].c_str()));

   // open log file
#if !defined(NOLOG)
   {
      char logfilename[1024];

      sprintf(logfilename, "%s.%d", logFileName.c_str(), crawler_id);
      CreateLog(logfilename, 100*1024*1024);
   }
#endif

   // status file - <wlam@cs.stanford.edu> Feb 2001
   CreateStatusFile();

#if defined(HISTORY)
   {
      char historyfilename[1024];
      time_t     now_t;
      struct tm  now;

      now_t = time(NULL);
      localtime_r(&now_t, &now);

      sprintf(historyfilename, "%s.%02d%02d", HISTORYFILENAME,
	      now.tm_mon+1, now.tm_mday);

      _HISTORYFILE = fopen(historyfilename, "a+");
      if (_HISTORYFILE == NULL) {
	 cerr << "Cannot create history file\n";
	 exit(1);
      }
   }
#endif
   
   {
     //Pranav start

     //     ifstream is(argv[3]);
     ifstream is;
     if(strConfig.size() != 0) {
	 char tmpName[256];
         strcpy(tmpName, strConfig.c_str());
         strcat(tmpName, ".");
         strcat(tmpName, itoa(crawler_id).c_str());
	 fprintf(stderr, "disk config file: %s\n", tmpName);
         is.open(tmpName);
     }
     else {
       cerr << "Crawlserver:: error in config file\n";
       exit(1);
     }
     //cout << "config file: " << strConfig << endl; //debug
     //Pranav end

      string disk;
      disklist = new vector<string>;

      is >> disk;
      while (is) {
	 disklist->push_back(disk);
	 is >> disk;
      }
   }

   //cout << "crawlerid: " << crawler_id << endl;   //debug
   strcpy(repName, "repository");
   strcat(repName, ".");
   strcat(repName, itoa(crawler_id).c_str());
   manager  *mg = new manager(crawler_id, local_addr, site_addr, archiver_addr);
   
#ifdef SOCKET_CONTROL
   if (daemonize) daemon(1 /* don't chdir */ ,0);
#endif

   mg->main_loop();

   delete mg;

   return 0;
}

int finish_process()
{
   exit(1);
}
















