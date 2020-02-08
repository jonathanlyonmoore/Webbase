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
#include <string>
#include <set>

#include "my_utility.h"
#include "url_filter.h"

set<string> url_filter::ext_parse;
set<string> url_filter::ext_store;
bool url_filter::f_img_enabled = false;

void url_filter::load_ext_set(set<string>& ext_set, const vector<string>& list) {
  vector<string>::const_iterator iter;
  for(iter = list.begin(); iter != list.end(); iter++) {
    // check for special extensions
    if(*iter == "no_ext") {
      ext_set.insert("");
      continue;
    }

    ext_set.insert(*iter);
  }
}

int  url_filter::add_seed_filter(int seed_id, int level, int count)
{
   seed_entry entry = { seed_id, level, count };
   seed_filter.push_back(entry);

   return 0;
}

int url_filter::add_prefix_filter(const string& prefix)
{
   prefix_filter.push_back(prefix);
   return 0;
}

int url_filter::add_substr_filter(const string& substr)
{
   substr_filter.push_back(substr);
   return 0;
}

bool url_filter::parse_enabled(const url* url) {
  const char* ext = get_extension(url);
  bool r = (ext_parse.find(ext) != ext_parse.end());
  //  cerr << "parse_enabled for " << url->path << " with ext " << ext << " set to " << r << endl;
  return r;
}

bool url_filter::store_enabled(const url* url) {
  const char* ext = get_extension(url);
  bool r = (ext_store.find(ext) != ext_store.end());
  //  cerr << "store_enabled for " << url->path << " with ext " << ext << " set to " << r << endl;
  return r;
}

const char* url_filter::get_extension(const url* url) {
  const char* path        = url->path;
  int len = strlen(path);
  const char* ext_end     = path + len - 1;
  const char* ext_start;
  char c = '*';

  for (ext_start = ext_end; path < ext_start; --ext_start) {
    c = *ext_start;
    if (c == '.' || c == '/') break;
  }
  if (c == '.') {
    int ext_length = ext_end - ++ext_start;
    if (ext_length > 5 || ext_length <= 0) return "";
    return ext_start;
  }
  return "";
}

int url_filter::check(const url* url, 
		      int seed_id, int level, int count) const
{
   const char* path        = url->path;
   int         path_length = strlen(path);

   //
   // check extension
   //
#if defined(SHIVA)
   if (strlen(url->query) > 0) goto check_prefix; 
#endif

   // if we don't need to follow or store the url, then we drop it
   if(!parse_enabled(url) && !store_enabled(url) )
     return RC_UF_INVALID_EXTENSION;
   
   //
   // check prefix filter
   //
#if defined(SHIVA)
 check_prefix:
   for (int i = 0; (unsigned)i < prefix_filter.size(); i++) {
      int prefix_length = prefix_filter[i].length();
      if (prefix_length <= path_length) {
	 if (strncmp(prefix_filter[i].c_str(), path, 
		     prefix_length) == 0) {
	    goto check_substr;
	 }
      }
      return RC_UF_INVALID_PREFIX;
   }
#else
   for (int i = 0; (unsigned)i < prefix_filter.size(); i++) {
      int prefix_length = prefix_filter[i].length();
      if (prefix_length <= path_length) {
	 if (strncmp(prefix_filter[i].c_str(), path, 
		     prefix_length) == 0) {
	    return RC_UF_INVALID_PREFIX;
	 }
      }
   }
#endif
   //
   // check substr filter
   //
#ifdef SHIVA
  check_substr:
#endif // SHIVA
   for (int i = 0; (unsigned)i < substr_filter.size(); i++) {
      if (strstr(path, prefix_filter[i].c_str()) != NULL) {
	 return RC_UF_INVALID_SUBSTR;
      }
   }

   //
   // check seed filter
   //
   for (int i = 0; (unsigned)i < seed_filter.size(); i++) {
      if (seed_filter[i].seed_id == seed_id) {
	 if ((seed_filter[i].level > 0 && seed_filter[i].level < level)) {
	    return RC_UF_MAX_LEVEL;
	 } 
	 if (seed_filter[i].count > 0 && seed_filter[i].count < count) {
	    return RC_UF_MAX_COUNT;
	 }
      }
   }
   
   return 0;
}

int url_filter::reset()
{
   prefix_filter.erase(prefix_filter.begin(), prefix_filter.end());
   substr_filter.erase(substr_filter.begin(), substr_filter.end());
   seed_filter.erase(seed_filter.begin(), seed_filter.end());

   return 0;
}

int url_filter::save(ostream& os, const char *name) const
{
   os << name << "Start\n";

   if (prefix_filter.size() > 0) {
      writelist(os, "PrefixFilter", prefix_filter);
   }
   if (substr_filter.size() > 0) {
      writelist(os, "SubstrFilter", substr_filter);
   }
   if (seed_filter.size() > 0) {
      writelist(os, "SeedFilter", seed_filter);
   }

   os << name << "End\n";

   return 0;
}

int url_filter::load(istream& is, const char *name)
{
   const int   maxline = 2*1024;
   char        line[maxline];
   string      start, end;

   // initialize filters
   reset();

   // start and end marker
   start = name; start += "Start";
   end   = name; end   += "End";
   
   // skip start marker
   is >> ws; readline(is, line, maxline);
   if (strcmp(line, start.c_str()) == 0) {
      is >> ws; readline(is, line, maxline);
   }

   while (!is.eof()) {
      if (strcmp(line, "PrefixFilterStart") == 0) {
	 if (readlist(is, "PrefixFilter", prefix_filter) < 0) {
	    return RC_UF_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp(line, "SubstrFilterStart") == 0) {
	 if (readlist(is, "SubstrFilter", substr_filter) < 0) {
	    return RC_UF_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp(line, "SeedFilterStart") == 0) {
	 if (readlist(is, "SeedFilter", seed_filter) < 0) {
	    return RC_UF_INVALID_FILE_FORMAT;
	 }
      } else if (strcmp(line, end.c_str()) == 0) {
	 return 0;
      }

      is >> ws; readline(is, line, maxline);
   }

   return RC_UF_INVALID_FILE_FORMAT;
}
