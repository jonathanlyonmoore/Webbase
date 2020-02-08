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
#ifndef PUTASIDE_QUEUE_H
#define PUTASIDE_QUEUE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <map>
#include <string>
#include "url.h"

const int RC_PQ_ERROR_START    =  -400;
const int RC_PQ_QUEUE_OVERFLOW =  RC_PQ_ERROR_START - 1;


class putaside_queue {
   public:
      typedef std::map<std::string,int>::const_iterator  const_queue_iterator;
      typedef std::map<std::string,int>::iterator        queue_iterator;

      putaside_queue(int max_entries = 10000);
      ~putaside_queue();

      int put_url(const url& u);
      int get_size() const;
      int purge_url(std::string& url_list);

   private:
      std::map<std::string,int>  site_queue;

      int  max_num_entries;
};


#endif // PUTASIDE_QUEUE_H
