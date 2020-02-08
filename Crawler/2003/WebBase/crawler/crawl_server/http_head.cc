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
#include <strings.h>

time_t readdate(const char* s)
{
   static const char* monthlist[] = {
      "Jan", "Feb", "Mar", "Apr", "May", "Jun", 
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

   char weekday[20], month[20], zone[20];
   struct tm tm_date;
   time_t    date;

   if (strchr(s, ',') == NULL) {
      sscanf(s, "%s %s %d %02d:%02d:%02d %d",
	     &weekday, &month, &tm_date.tm_mday, 
	     &tm_date.tm_hour, &tm_date.tm_min, &tm_date.tm_sec,
	     &tm_date.tm_year);
      tm_date.tm_year -= 1900;
   } else {
      if (strchr(s, '-') == NULL) {
	 sscanf(s, "%s %02d %s %04d %02d:%02d:%02d %s",
		&weekday, &tm_date.tm_mday, &month, &tm_date.tm_year,
		&tm_date.tm_hour, &tm_date.tm_min, &tm_date.tm_sec,
		&zone);
	 tm_date.tm_year -= 1900;
      } else {
	 sscanf(s, "%s %02d%5s%02d %02d:%02d:%02d %s",
		&weekday, &tm_date.tm_mday, &month, &tm_date.tm_year,
		&tm_date.tm_hour, &tm_date.tm_min, &tm_date.tm_sec, &zone);
	 memmove(month, month+1, 3);
	 month[4] = 0;
      }
   }

   for (int i = 0; i < sizeof(monthlist)/sizeof(char*); i++) {
      if (strncasecmp(month, monthlist[i], 3) == 0) {
	 tm_date.tm_mon = i;
	 break;
      }
   }

   tm_date.tm_isdst = -1;
   date = mktime(&tm_date) - timezone;

   struct tm* newtime;
   newtime = gmtime(&date);
   printf("%s%d/%d/%d %d:%d:%d\n", s,
	  newtime->tm_mon, newtime->tm_mday, newtime->tm_year,
	  newtime->tm_hour, newtime->tm_min, newtime->tm_sec);

   return date;
}

main()
{
   char line[2048];

   while (fgets(line, 2048, stdin) != NULL) {
      if (strncmp(line, "Date: ", 6) == 0) {
	 readdate(line + 6);
      }
   }
}
	 
