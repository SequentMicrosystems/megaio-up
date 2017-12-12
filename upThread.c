/*
 * UpThread.c:
 *	Provide a simplified interface to pthreads
 *
 *	Copyright (c) 2016-2017 Sequent Microsystem
 *	<http://www.sequentmicrosystems.com>
 ***********************************************************************
 * This file is part of megaio:
 *
 *    megaio is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as
 *    published by the Free Software Foundation, either version 3 of the
 *    License, or (at your option) any later version.
 *
 *    megaio is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with megaio.
 *    If not, see <http://www.gnu.org/licenses/>.
 ***********************************************************************
 */

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sched.h>
#include "megaio.h"

static pthread_mutex_t piMutexes [4] ;



/*
 * upHiPri:
 *	Attempt to set a high priority schedulling for the running program
 *********************************************************************************
 */

int upHiPri (const int pri)
{
  struct sched_param sched ;

  memset (&sched, 0, sizeof(sched)) ;

  if (pri > sched_get_priority_max (SCHED_RR))
    sched.sched_priority = sched_get_priority_max (SCHED_RR) ;
  else
    sched.sched_priority = pri ;

  return sched_setscheduler (0, SCHED_RR, &sched) ;
}


/*
 * upThreadCreate:
 *	Create and start a thread
 *********************************************************************************
 */

int upThreadCreate (void *(*fn)(void *))
{
  pthread_t myThread ;

  return pthread_create (&myThread, NULL, fn, NULL) ;
}

/*
 * upLock: upUnlock:
 *	Activate/Deactivate a mutex.
 *	We're keeping things simple here and only tracking 4 mutexes which
 *	is more than enough for out entry-level pthread programming
 *********************************************************************************
 */

void upLock (int key)
{
  pthread_mutex_lock (&piMutexes [key]) ;
}

void upUnlock (int key)
{
  pthread_mutex_unlock (&piMutexes [key]) ;
}

