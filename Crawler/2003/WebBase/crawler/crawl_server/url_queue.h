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
#ifndef URL_QUEUE_H
#define URL_QUEUE_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <iostream>
//#include <bvector.h>
// #include <hash_set>
#include <deque>
#include "url.h"

const int      MAX_NUM_URLS  = 1024*1024;
//const unsigned URL_HASH_MASK = 0xfffff;
const unsigned URL_HASH_MASK = 0xffffffff;

const int RC_QM_ERROR_START            = -200;
const int RC_QM_URL_WRONG_QUEUE        = RC_QM_ERROR_START - 1;
const int RC_QM_URL_FILTERED_OUT       = RC_QM_ERROR_START - 2;
const int RC_QM_URL_SEEN_BEFORE        = RC_QM_ERROR_START - 3;
const int RC_QM_QUEUE_EMPTY            = RC_QM_ERROR_START - 4;
const int RC_QM_CANNOT_ALLOCATE_MEMORY = RC_QM_ERROR_START - 5;
const int RC_QM_TOO_MANY_REQUESTS      = RC_QM_ERROR_START - 6;
const int RC_QM_INVALID_FILE_FORMAT    = RC_QM_ERROR_START - 7;

class url_queue {
   public:
      struct url_info {
	    const url* u;
	    int        seed_id;
	    int        level;
      };

      url_queue();
      ~url_queue();

      int              push_url(const url_info& info);
      const url_info&  top_url() const;
      int              pop_url();
      int              is_empty() const;

      int              reset();
      int              save(std::ostream& out, const char* name = "UrlQueue");
      int              load(std::istream& in,  const char* name = "UrlQueue");

   private:
      // url queue and url table
      int                   seen_count;
      //      std::bit_vector       *urls_seen;
      hash_set<unsigned> urls_seen;
      std::deque<url_info>  urls_to_crawl;

      unsigned hash_url(const url* u) const;
};

inline std::ostream& operator << (std::ostream& os,
                                  const url_queue::url_info& ui)
{
   os << *ui.u << ' ' << ui.seed_id << ' ' << ui.level;
   return os;
}

inline std::istream& operator >> (std::istream& is, url_queue::url_info& ui)
{
   ui.u = new dynamic_url;
   is >> *(dynamic_url*)ui.u >> ui.seed_id >> ui.level;
   return is;
}


#endif // URL_QUEUE_H
