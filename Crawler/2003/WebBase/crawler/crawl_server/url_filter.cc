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
#include <vector>
#include <sstream>

#include "my_utility.h"
#include "url_filter.h"
#include "TypeDetector.h"

using std::string;
using std::set;
using std::vector;

set<TypeDetector::Type> url_filter::types_parse;
set<TypeDetector::Type> url_filter::types_store;

bool url_filter::f_img_enabled = false;
TypeDetector url_filter::typeDetector;

void url_filter::load_types(set<TypeDetector::Type>& types, 
			    const vector<string>& type_names) {
  TypeDetector::init();
  
  vector<string>::const_iterator iter;
  for(iter = type_names.begin(); iter != type_names.end(); iter++) {
    if(TypeDetector::NameTypes.find(*iter) != TypeDetector::NameTypes.end()) {
       types.insert(TypeDetector::NameTypes[*iter]);
    }
    else {
      error() << "Unknown typename: " << *iter << endl;
      exit(1);
    }
  }
}

int  url_filter::add_seed_filter(int seed_id, int level, int count)
{
   seed_entry entry = { seed_id, level, count };
   seed_filter.push_back(entry);

   return 0;
}

int url_filter::add_prefix_filter(const std::string& prefix)
{
   prefix_filter.push_back(prefix);
   return 0;
}

int url_filter::add_substr_filter(const std::string& substr)
{
   substr_filter.push_back(substr);
   return 0;
}

bool url_filter::is_enabled(set<TypeDetector::Type>& types_set, 
			    const url* url) {
  if (types_set.find(TypeDetector::ANY) != types_set.end())
    return true;

  TypeDetector::Type type = typeDetector.getTypeOf(url, NULL, 0);
  bool r = (types_set.find(type) != types_set.end());
  //  cerr << "enabled for " << url->path << " with type " << TypeDetector::TypeNames[type] << endl;
  return r;
}
bool url_filter::parse_enabled(const url* url) {
  return is_enabled(types_parse, url);
}

bool url_filter::store_enabled(const url* url) {
  return is_enabled(types_store, url);
}

int url_filter::check(const url* url, 
		      int seed_id, int level, int count) const
{
   // For completeness, match by more of URL than path component alone.
   // --wlam, 8 Aug 2003
   // const char* path        = url->path;
   std::ostringstream url_stream;  
   {
      // Below is summarized from url.cc; do not use ostream&<<(ostream&,url&)
      // directly because we must omit 'scheme://net_loc:port' part, but
      // please update the below if ostream&<<(ostream&,url&) is changed.
      if (strlen(url->path) > 0) { url_stream << url->path; }
      if (strlen(url->param) > 0) { url_stream << ';' << url->param; }
      if (strlen(url->query) > 0) { url_stream << '?' << url->query; }
   }
   const char* path        = url_stream.str().c_str();
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

int url_filter::save(std::ostream& os, const char *name) const
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

int url_filter::load(std::istream& is, const char *name)
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
   is >> std::ws; readline(is, line, maxline);
   if (strcmp(line, start.c_str()) == 0) {
      is >> std::ws; readline(is, line, maxline);
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

      is >> std::ws; readline(is, line, maxline);
   }

   return RC_UF_INVALID_FILE_FORMAT;
}
