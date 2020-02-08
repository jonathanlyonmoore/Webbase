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

#ifndef _DAEMON_H
#define _DAEMON_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_DAEMON

/* This is a hack nonworking implementation of daemon.
   In particular, it should return error codes correctly.
*/
int daemon(int nocd, int noclose);

#endif /* #ifndef HAVE_DAEMON */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _DAEMON_H */

