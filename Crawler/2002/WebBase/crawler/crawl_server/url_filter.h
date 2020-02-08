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
#ifndef URL_FILTER_H
#define URL_FILTER_H

#include <vector>
#include <string>
#include <set>

#include "url.h"

const int RC_UF_ERROR_START = -1000;
const int RC_UF_INVALID_EXTENSION    = RC_UF_ERROR_START - 1;
const int RC_UF_INVALID_PREFIX       = RC_UF_ERROR_START - 2;
const int RC_UF_INVALID_SUBSTR       = RC_UF_ERROR_START - 3;
const int RC_UF_MAX_LEVEL            = RC_UF_ERROR_START - 4;
const int RC_UF_MAX_COUNT            = RC_UF_ERROR_START - 5;
const int RC_UF_INVALID_FILE_FORMAT  = RC_UF_ERROR_START - 6;

class url_filter {
 public:
      struct seed_entry {
	    int  seed_id;
	    int  level;
	    int  count;
      };

 private: 
      vector<string>     prefix_filter;
      vector<string>     substr_filter;
      vector<seed_entry> seed_filter;

      // create a set for each action, to keep track of which extensions
      // are triggers
      static set<string> ext_parse;
      static set<string> ext_store;

      static bool f_img_enabled;

   public:
      int  add_seed_filter(int seed_id, int level, int count);
      int  add_prefix_filter(const string& prefix);
      int  add_substr_filter(const string& substr);
      int  check(const url* url, int seed_id, int level, int count) const;
      
      int  reset();
      int  save(ostream& os, const char* name = "UrlFilter") const;
      int  load(istream& is, const char* name = "UrlFilter");

      static void  set_store_list(const vector<string>& list) { 
	load_ext_set(ext_store, list);
      }
      static void  set_parse_list(const vector<string>& list) {
	load_ext_set(ext_parse, list);
      }
      static void load_ext_set(set<string>& ext_set, const vector<string>& list);

      static bool parse_enabled(const url* url);
      static bool store_enabled(const url* url);

      static bool img_enabled() { return f_img_enabled; }
      static void set_img_enabled(bool f) { f_img_enabled = f; }
 private:
      static const char* get_extension(const url* url);
};

inline ostream& operator << (ostream& os, const url_filter::seed_entry& se)
{
   os << se.seed_id << ' ' << se.level << ' ' << se.count;
   return os;
}

inline istream& operator >> (istream& is, url_filter::seed_entry& se)
{
   is >> se.seed_id >> se.level >> se.count;
   return is;
}

#endif // URL_FILTER_H
