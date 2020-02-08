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

/*
 * TUtils.cc
 *
 * Utils that require pthreads
 */

#include "TUtils.h"

#include "Utils.h"

void TUtils::threadSleep(long long millis) {

  if(millis <= 0)
    return;

  long long wakeup = Utils::currentTimeMillis() + millis; // absolute time to wake up
  
  struct timespec t;
  t.tv_sec = wakeup / 1000;
  t.tv_nsec = (wakeup % 1000) * 1000000; // from millis to nanos

  pthread_cond_t cvp;
  pthread_mutex_t mutex;
  pthread_cond_init(&cvp, NULL);
  pthread_mutex_init(&mutex, NULL);

  //  cout << "GOING to sleep" << endl;
  pthread_cond_timedwait(&cvp, &mutex, &t);
  //  cout << "AWAKEN!" << endl;
}
