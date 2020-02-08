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
#ifndef MY_UTILITY_H
#define MY_UTILITY_H

#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <strstream>

// BEGIN SOLARIS PORT CHANGE
#include <time.h>
// END SOLARIS PORT CHANGE


istream& readline(istream& in, char* p, int n);
istream& readline(istream& in, string& line);

template <class T>
int writeval(ostream& os, const char* name, const T& val)
{
   os << name << "Start\n";
   os << val << endl;
   os << name << "End\n";

   return 0;
}

template <class T>
int readval(istream& is, const char* name, T& val)
{
   string  line;
   string  start, end;

   // start and end marker
   start = name; start += "Start";
   end   = name; end   += "End";

   // skip the start marker
   is >> ws; readline(is, line);
   if (line.compare(start) == 0) {
      is >> ws; readline(is, line);
   }

   // read the value
   istrstream ss(line.data(), line.length());
   ss >> val;

   // skip the end marker
   is >> ws; readline(is, line);
   if (line.compare(end) != 0) {
      return -1;
   }

   return 0;
}

template <class List>
int writelist(ostream& os, const char* listName, const List& ls)
{
   typedef typename List::const_iterator  iterator;
   iterator  i;

   os << listName << "Start\n";

   for (i = ls.begin(); i != ls.end(); i++) {
      os << *i << endl;
   }

   os << listName << "End\n";

   return 0;
}

template <class List>
int readlist(istream& is, const char* listName, List& ls)
{
   typedef typename List::value_type      value_type;
   string      line, start, end;
   value_type  elm;
   int         count = 0;

   // erase all elements in the list
   ls.erase(ls.begin(), ls.end());
   
   // start and end marker
   start = listName; start += "Start";
   end   = listName; end   += "End";

   // skip the start marker
   is >> ws; readline(is, line);
   if (line.compare(start) == 0) {
      is >> ws; readline(is, line);
   }

   // read elements of the list
   while (!is.eof()) {
      if (line.compare(end) == 0) {
	 return count;
      }

      istrstream ss(line.data(), line.length());

      ss >> elm;
      ls.push_back(elm);
      count++;

      is >> ws; readline(is, line);
   }

   return -1;
}

//
// specialization of writelist/readlist for bit_vector
//
int writebvec(ostream& os, const char* listName, const bit_vector& ls);
int readbvec(istream& is, const char* listName, bit_vector& ls);

const string itoa(int i);
void to_lower(string& s);
unsigned hash_string(const char* str);

int split_string(const string& s, const char* separators, 
		 vector<string>& list);
int split_string2(const string& s, const char* separators, 
		  vector<string>& list);

template<class T, int size>
struct table {
   public:
      inline table() {
	 cursor = size - 1; 
	 for (int i = 0; i < size; i++) {
	    data[i].erase();
	 }
      }
      inline int allocate() {
	 register int i = cursor;
	 do {
	    i = (i+1) % size;
	    if (data[i].empty())  break;
	 } while (i != cursor);
	 if (data[i].empty()) {
	    cursor = i;
	    return i;
	 }
	 return -1;
      }
      inline void erase(int entry) { 
	 data[entry].erase(); 
      }
      inline T& operator [] (int entry) const { 
	 assert(entry >= 0 && entry < size);
	 return data[entry]; 
      }
   private:
      T data[size];
      int  cursor;
};
      

extern const char* _filename;
extern int         _linenumber;
extern const char* _logfilename;
extern int         _maxlogline;
void _PrintLog(const char* fmt, ...);


extern FILE* _LOGFILE;
int  CreateLog(const char* name, int maxsize);
#define LOG(x)  { _filename = __FILE__; _linenumber = __LINE__; \
                   _PrintLog x; } 

#endif // MY_UTILITY
