/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2017 Luca Baldesi
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
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <chunk.h>
#include <chunkiser.h>
#include <psinstance_internal.h>
#include <measures.h>

#include "output.h"
#include "dbg.h"

struct chunk_output {
	int last_chunk;
	int next_chunk;
	int buff_size;
	int start_id;
	int end_id;
	
	char sflag;
	char eflag;
	struct chunk *buff;
	struct output_stream *out;
	const struct psinstance * ps;
	
	bool reorder; 
};

//struct outbuf {
//  struct chunk c;
//};
//static struct outbuf *buff;
//static struct output_stream *out;


void output_init(struct chunk_output * outg, int bufsize, const char *config)
{
  //char * c;

  // c = strchr(config,',');  // this is actually ignored
  // if (c) {
  //   *(c++) = 0;
  // }
  outg->out = out_stream_init("/dev/stdout", config);
  if (outg->out == NULL) {
     fprintf(stderr, "Error: can't initialize output module\n");
     exit(1);
  }
  if (!outg->buff) {
    int i;

    outg->buff_size = bufsize;
    outg->buff = malloc(sizeof(struct chunk) * outg->buff_size);
    if (!outg->buff) {
     fprintf(stderr, "Error: can't allocate output buffer\n");
     exit(1);
    }
    for (i = 0; i < outg->buff_size; i++) {
      (outg->buff)[i].data = NULL;
    }
  } else {
   fprintf(stderr, "Error: output buffer re-init not allowed!\n");
   exit(1);
  }
	outg->last_chunk = -1;
	outg->next_chunk = -1;
	outg->sflag = 0;
	outg->eflag = 0;
	outg->start_id = -1;
	outg->end_id = -1;
	outg->reorder = true; // should be zero if using chunkstream (legacy code from PeerStreamer)
}

static void buffer_print(const struct chunk_output * outg)
{
#ifdef DEBUG
  int i;

  if (outg->next_chunk < 0) {
    return;
  }

  dprintf("\toutbuf: %d-> ",outg->next_chunk);
  for (i = outg->next_chunk; i < outg->next_chunk + outg->buff_size; i++) {
    if (outg->buff[i % outg->buff_size].data) {
      dprintf("%d",i % 10);
    } else {
      dprintf(".");
    }
  }
  dprintf("\n");
#endif
}

static void buffer_free(struct chunk_output * outg, int i)
{
  dprintf("\t\tFlush Buf %d: %s\n", i, outg->buff[i].data);
  if(outg->start_id == -1 || outg->buff[i].id >= outg->start_id) {
    if(outg->end_id == -1 || outg->buff[i].id <= outg->end_id) {
      if(outg->sflag == 0) {
        fprintf(stderr, "\nFirst chunk id played out: %d\n\n",outg->buff[i].id);
        outg->sflag = 1;
      }
      if (outg->reorder) chunk_write(outg->out, &(outg->buff[i]));
      outg->last_chunk = outg->buff[i].id;
    } else if (outg->eflag == 0 && outg->last_chunk != -1) {
      fprintf(stderr, "\nLast chunk id played out: %d\n\n", outg->last_chunk);
      outg->eflag = 1;
    }
  }

  free(outg->buff[i].data);
  outg->buff[i].data = NULL;
  dprintf("Next Chunk: %d -> %d\n", outg->next_chunk, outg->buff[i].id + 1);
  reg_chunk_playout(psinstance_measures(outg->ps), outg->buff[i].id, true, outg->buff[i].timestamp);
  outg->next_chunk = outg->buff[i].id + 1;
}

static void buffer_flush(struct chunk_output * outg, int id)
{
  int i = id % outg->buff_size;

  while(outg->buff[i].data) {
    buffer_free(outg, i);
    i = (i + 1) % outg->buff_size;
    if (i == id % outg->buff_size) {
      break;
    }
  }
}

void output_deliver(struct chunk_output * outg, const struct chunk *c)
{
  if (!outg->buff) {
    fprintf(stderr, "Warning: buff not initialized!!! Setting output buffer to 8\n");
  }

  if (!outg->reorder) chunk_write(outg->out, c);

  dprintf("Chunk %d delivered\n", c->id);
  buffer_print(outg);
  if (c->id < outg->next_chunk) {
    return;
  }

  /* Initialize buffer with first chunk */
  if (outg->next_chunk == -1) {
    outg->next_chunk = c->id; // FIXME: could be anything between c->id and (c->id - buff_size + 1 > 0) ? c->id - buff_size + 1 : 0
    fprintf(stderr,"First RX Chunk ID: %d\n", c->id);
  }

  if (c->id >= outg->next_chunk + outg->buff_size) {
    int i;

    /* We might need some space for storing this chunk,
     * or the stored chunks are too old
     */
    for (i = outg->next_chunk; i <= c->id - outg->buff_size; i++) {
      if (outg->buff[i % outg->buff_size].data) {
        buffer_free(outg, i % outg->buff_size);
      } else {
        reg_chunk_playout(psinstance_measures(outg->ps), c->id, false, c->timestamp); // FIXME: some chunks could be counted as lost at the beginning, depending on the initialization of next_chunk
        outg->next_chunk++;
      }
    }
    buffer_flush(outg, outg->next_chunk);
    dprintf("Next is now %d, chunk is %d\n", outg->next_chunk, c->id);
  }

  dprintf("%d == %d?\n", c->id, outg->next_chunk);
  if (c->id == outg->next_chunk) {
    dprintf("\tOut Chunk[%d] - %d: %s\n", c->id, c->id % outg->buff_size, c->data);

    if(outg->start_id == -1 || c->id >= outg->start_id) {
      if(outg->end_id == -1 || c->id <= outg->end_id) {
        if(outg->sflag == 0) {
          fprintf(stderr, "\nFirst chunk id played out: %d\n\n",c->id);
          outg->sflag = 1;
        }
        if (outg->reorder) chunk_write(outg->out, c);
        outg->last_chunk = c->id;
      } else if (outg->eflag == 0 && outg->last_chunk != -1) {
        fprintf(stderr, "\nLast chunk id played out: %d\n\n", outg->last_chunk);
        outg->eflag = 1;
      }
    }
    reg_chunk_playout(psinstance_measures(outg->ps), c->id, true, c->timestamp);
    outg->next_chunk++;
    buffer_flush(outg, outg->next_chunk);
  } else {
    dprintf("Storing %d (in %d)\n", c->id, c->id % outg->buff_size);
    if (outg->buff[c->id % outg->buff_size].data) {
      if (outg->buff[c->id % outg->buff_size].id == c->id) {
        /* Duplicate of a stored chunk */
        dprintf("\tDuplicate!\n");
        reg_chunk_duplicate(psinstance_measures(outg->ps));
        return;
      }
      fprintf(stderr, "Crap!, chunkid:%d, storedid: %d\n", c->id, outg->buff[c->id % outg->buff_size].id);
      exit(-1);
    }
    /* We previously flushed, so we know that c->id is free */
    memcpy(&(outg->buff[c->id % outg->buff_size]), c, sizeof(struct chunk));
    outg->buff[c->id % outg->buff_size].data = malloc(c->size);
    memcpy(outg->buff[c->id % outg->buff_size].data, c->data, c->size);
  }
}

struct chunk_output * output_create(int bufsize, const char *config, const struct psinstance * ps)
{
	struct chunk_output * outg;

	outg = malloc(sizeof(struct chunk_output));
	outg->ps = ps;
	outg->buff = NULL;
	outg->out = NULL;
	output_init(outg, bufsize, config);

	return outg;
}

void output_destroy(struct chunk_output ** outg)
{
	if (outg && *outg)
	{
		if((*outg)->buff)
			free((*outg)->buff);
		if((*outg)->out)
			out_stream_close((*outg)->out);
		free(*outg);
	}
}
