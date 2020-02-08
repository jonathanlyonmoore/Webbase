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
#ifndef HTML_PARSER_H
#define HTML_PARSER_H

class html_parser {
   public:
      static const int AREA   = 100;
      static const int A      = 101;
      static const int BASE   = 102;
      static const int LINK   = 103;
      static const int FRAME  = 104;
      static const int IFRAME = 105;
      static const int IMG    = 106;

      int start_parse(const char* buffer_start, const char* buffer_end);
      int end_parse();
      int get_link(const char*& start, const char*& end);
      
   private:
      const char* buffer_start;
      const char* buffer_end;
      const char* buffer_pointer;

      static const char *tag_string[];
      static const int tag_length[]; 
      static const int tag_token[]; 

      int find_tag(const char*& start, const char*& end);
      int find_url(const char*& start, const char*& end);
      const char* find_substr(const char* begin, const char* end, 
			      const char* substr);
      inline char normalize_ws(char c);
      inline char to_lower(char c);
};

#endif /* HTML_PARSER_H */
