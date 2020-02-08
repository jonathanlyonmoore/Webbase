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
#include "putaside_queue.h"
#include "my_utility.h"
#include <ctype.h>

putaside_queue::putaside_queue(int max_entries)
{
   max_num_entries = max_entries;
}

putaside_queue::~putaside_queue()
{}


int putaside_queue::put_url(const url& u)
{
   // check whether the site name is valid
   register char c;
   register const char *s;
   for (s = u.net_loc; (c = *s) != 0; s++) {
      if (isspace(c)) return 0;
   }

   // construct the site name from the url
   string site_name(u.net_loc);
   if (u.port != NULL && *u.port != 0 && strcmp(u.port, "80") != 0) {
      site_name.append(":");
      site_name.append(u.port);
   }

   // check whether queue overflows
   if (site_queue.size() >= (unsigned)max_num_entries) {
      if (site_queue.find(site_name) == site_queue.end()) {
	 return RC_PQ_QUEUE_OVERFLOW;
      }
   }

   // insert the site name to the queue
   site_queue[site_name]++;

   return 0;
}

int putaside_queue::get_size() const
{
   return site_queue.size();
}

int putaside_queue::purge_url(string& url_list)
{
   const_queue_iterator  iter;

   url_list = "";

   // first print all the entries in the queue
   LOG(("Purging putasde queue..............."));
   for (iter = site_queue.begin(); iter != site_queue.end(); iter++) {
      url_list += iter->first;
      url_list += " ";
      url_list += itoa(iter->second);
      url_list += "\r\n";
   }
   LOG(("Done purging........................"));
   
   // erase all entries
   site_queue.erase(site_queue.begin(), site_queue.end());

   return 0;
}
