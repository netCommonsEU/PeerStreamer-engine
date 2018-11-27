/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2017-2018 Luca Baldesi
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
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include<grapes_config.h>

#include <chunk.h>
#include <chunkiser.h>

#include "input.h"
#include "dbg.h"
#include<chunk_attributes.h>

#define INITIAL_ID 0
#define DEFAULT_DATA_INTERVAL 30000

struct input_desc {
  struct input_stream *s;
  int id;
  int interframe;
  uint64_t start_time;
  uint64_t first_ts;
};

struct input_desc *input_open(struct input_context * ctx, const char * config)
{
  struct input_desc *res = NULL;
  struct timeval tv;

  res = malloc(sizeof(struct input_desc));
  memset(res, 0, sizeof(struct input_desc));

  res->s = input_stream_open(ctx->filename, &res->interframe, config);
  if (res->s)
  {
	  if (res->interframe == 0) {
		const int *my_fds;
		int i = 0;

		my_fds = input_get_fds(res->s);
		while(my_fds[i] != -1) {
		  ctx->fds[i] = my_fds[i];
		  i = i + 1;
		}
		ctx->fds[i] = -1;
	  } else {
		if (ctx->fds_size >= 1) {
		  ctx->fds[0] = -1; //This input module needs no fds to monitor
		}
		gettimeofday(&tv, NULL);
		res->start_time = tv.tv_usec + tv.tv_sec * 1000000ULL;
		res->first_ts = 0;
		res->id = 0; //(res->start_time / res->interframe) % INT_MAX; //TODO: verify 32/64 bit

		if(INITIAL_ID == -1) {
		  res->id = (res->start_time / res->interframe) % INT_MAX; //TODO: verify 32/64 bit
		} else {
		  res->id = INITIAL_ID;
		}

		fprintf(stderr,"Initial Chunk Id %d\n", res->id);
	  }
  } else {
	  free(res);
	  res = NULL;
  }

  return res;
}

void input_close(struct input_desc *s)
{
  input_stream_close(s->s);
  free(s);
}

int input_get(struct input_desc *s, struct chunk *c)
{
  struct timeval now;
  int64_t delta;
  int res;

  c->attributes_size = 0;
  c->attributes = NULL;

  c->id = s->id;
  res = chunkise(s->s, c);
  if (res < 0) {
    return -1;
  }
  if (res > 0) {
    s->id++;
  }
  if (s->first_ts == 0) {
    s->first_ts = c->timestamp;
  }
  gettimeofday(&now, NULL);
  if (s->interframe) {
    delta = c->timestamp - s->first_ts + s->interframe;
//		fprintf(stderr,"delta  (%llu) = c->timestamp (%llu) - s->first_ts  (%llu) + s->interframe (%llu) \n",delta,c->timestamp,s->first_ts,s->interframe);
    delta = delta + s->start_time - now.tv_sec * 1000000ULL - now.tv_usec;
//    fprintf(stderr,"delta  (%llu)= delta + s->start_time  (%llu)- now.tv_sec * 1000000ULL  (%lu)- now.tv_usec (%lu)\n",delta,s->start_time,now.tv_sec,now.tv_usec);
    dprintf("Delta: %ld\n", delta);
    dprintf("Generate Chunk[%d] (TS: %lu)\n", c->id, c->timestamp);
    if (delta < 0) {
      delta = 0;
    }
  } else {
    delta = DEFAULT_DATA_INTERVAL;
  }
//			if (c->data)
//				fprintf(stderr,"chunk size: %d  ",c->size);

  c->timestamp = now.tv_sec * 1000000ULL + now.tv_usec;

  return delta;
}

struct chunk *input_chunk(struct input_desc * s, suseconds_t *delta)
{
  struct chunk *c;

  c = malloc(sizeof(struct chunk));
  if (!c) {
    fprintf(stderr, "Memory allocation error!\n");
    return NULL;
  }
  memset(c, 0, sizeof(struct chunk));

  *delta = (suseconds_t)input_get(s, c);
  if (*delta < 0) {
    fprintf(stderr, "Error in input!\n");
    exit(-1);
  }
  if (c->data == NULL) {
    free(c);
    return NULL;
  }
  dprintf("Generated chunk %d of %d bytes\n",c->id, c->size);
  chunk_attributes_init(c);
  return c;
}
