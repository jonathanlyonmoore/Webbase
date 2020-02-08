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
#ifndef URL_H
#define URL_H

#include <iostream.h>

class url {
   public:
      const char *scheme;
      const char *net_loc;
      const char *port;
      const char *path;
      const char *param;
      const char *query;
      const char *fragment;

      url();
      int strcpy(char* b, const char* be) const;


   protected:
      url(const char*, const char*, const char*, 
	  const char*, const char*, const char*, const char*);
      void  initialize();
      char* parse(const char *start, const char* end, 
		  char* buffer, int buffer_size);
      char* merge(const url& base, char* buffer, int buffer_size);
      char* store(char* buffer, int buffer_size);

   private:
      url(const url&) {}
      void operator = (const url&) {}
};

class dynamic_url : public url {
   public:
      dynamic_url();
      dynamic_url(const char*, const char*, const char*, 
	  const char*, const char*, const char*, const char*);
      dynamic_url(const url& u);
      dynamic_url(const dynamic_url& u);
      void operator = (const url& u);
      void operator = (const dynamic_url& u);

      ~dynamic_url();

      int replace(const char*, const char*, const char*, 
		  const char*, const char*, const char*, const char*);
      int replace(const char* start, const char* end);
      int replace(const char* start, const char* end, const url& base);

   private:
      char  *buffer;
      int    buffer_size;
      char  *allocate_space();
};


class static_url : public url {
   public:
      static_url();
      static_url(const char*, const char*, const char*, 
	  const char*, const char*, const char*, const char*);
      static_url(const url& u);
      static_url(const static_url& u);
      void operator = (const url& u);
      void operator = (const static_url& u);

      int replace(const char*, const char*, const char*, 
		  const char*, const char*, const char*, const char*);
      int replace(const char* start, const char* end);
      int replace(const char* start, const char* end, const url& base);

   private:
      static const int max_url_length = 10*1024;
      char  buffer[max_url_length];
};

ostream& operator << (ostream& os, const url& u);
istream& operator >> (istream& is, dynamic_url& u);
istream& operator >> (istream& is, static_url& u);



#endif // URL_H
