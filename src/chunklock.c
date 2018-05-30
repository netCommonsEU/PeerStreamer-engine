/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2017 Luca Baldesi
 * Copyright (c) 2018 Massimo Girondi
 *
 * This file is part of PeerStreamer.
 *
 * PeerStreamer is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * PeerStreamer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with PeerStreamer.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "compatibility/timer.h"

#include "chunklock.h"

#include "net_helper.h"
#define LSIZE_INCREMENT 10


struct lock {
  int chunkid;
  int flowid;
  struct nodeID *peer;
  struct timeval timestamp;
};

struct chunk_locks{
	struct lock *locks;
	size_t lsize, lcount;
	struct timeval toutdiff;
};

void chunk_locks_destroy(struct chunk_locks ** cl)
{
	size_t i;

	if (cl && *cl)
	{
		if ((*cl)->locks)
		{
			for (i=0; i<(*cl)->lcount; i++) 
				if ((*cl)->locks[i].peer)
					nodeid_free((*cl)->locks[i].peer);
			free((*cl)->locks);
		}
		free(*cl);
		*cl = NULL;
	}

}

void locks_init(struct chunk_locks * cl)
{
  if (!cl->locks) {
    cl->lsize = LSIZE_INCREMENT;
    cl->locks = malloc(sizeof(struct lock) * cl->lsize);
    cl->lcount = 0;
  }

  if (!cl->locks) {
    fprintf(stderr, "Error allocating memory for locks!\n");
    exit(EXIT_FAILURE);
  }
}

struct chunk_locks * chunk_locks_create(uint32_t lock_timeout_ms)
{
	struct chunk_locks * cl;
	cl = malloc(sizeof(struct chunk_locks));
	cl->locks = NULL;
	cl->toutdiff.tv_sec = lock_timeout_ms/1000;
	cl->toutdiff.tv_usec = (lock_timeout_ms%1000)*1000;

	locks_init(cl);
	return cl;
}

int chunk_lock_timed_out(struct chunk_locks *cl, struct lock *l)
{
  struct timeval tnow,tout;
  gettimeofday(&tnow, NULL);
  timeradd(&l->timestamp, &(cl->toutdiff), &tout);

  return timercmp(&tnow, &tout, >);
}

void chunk_lock_remove(struct chunk_locks * cl, struct lock *l){
  if (l->peer)
	  nodeid_free(l->peer);
  memmove(l, l+1, sizeof(struct lock) * (cl->locks+(cl->lcount)-l-1));
  cl->lcount--;
}

void chunk_locks_cleanup(struct chunk_locks * cl){
  int i;

  for (i=(cl->lcount)-1; i>=0; i--) {
    if (chunk_lock_timed_out(cl, cl->locks+i)) {
      chunk_lock_remove(cl, cl->locks+i);
    }
  }
}

void chunk_lock(struct chunk_locks * cl, int flowid, int chunkid, struct peer *from){
  if (cl && from)
  {
	  cl->locks[cl->lcount].chunkid = chunkid;
	  cl->locks[cl->lcount].flowid  = flowid;
	  cl->locks[cl->lcount].peer = from ? nodeid_dup(from->id) : NULL;
	  gettimeofday(&((cl->locks)[cl->lcount].timestamp), NULL);
	  cl->lcount++;

	  if (cl->lcount == cl->lsize) {
		cl->lsize += LSIZE_INCREMENT;
		cl->locks = realloc(cl->locks , sizeof(struct lock) * cl->lsize);
	  }
  }
}

void chunk_unlock(struct chunk_locks * cl, int flowid, int chunkid){
  size_t i;

  if (cl)
  {
	  for (i=0; i<cl->lcount; i++) {
		if ((cl->locks)[i].chunkid == chunkid &&
                    (cl->locks)[i].flowid  == flowid) {
		  chunk_lock_remove(cl, (cl->locks)+i);
		  break;
		}
	  }
  }
}

int chunk_islocked(struct chunk_locks * cl, int flowid, int chunkid){
  size_t i;

  if(cl)
  {
	  chunk_locks_cleanup(cl);

	  for (i=0; i<cl->lcount; i++) {
		if ((cl->locks)[i].chunkid == chunkid &&
                    (cl->locks)[i].flowid  == flowid) {
		  return 1;
		}
	  }
  }
  return 0;
}
