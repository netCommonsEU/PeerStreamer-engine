/*
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
#include<grapes_config.h>

#include "output.h"
#include "dbg.h"


struct chunk_output {
	struct chunk *buff;
	struct output_stream *out;
	struct measures * measure;

	uint16_t buff_length;
	uint16_t head;
	uint8_t  reorder;
	int next_out;
};

void chunk_clone(struct chunk * dst, const struct chunk* src)
{
	dst->id = src->id;
	dst->size = src->size;
	dst->data = malloc(dst->size);
	memmove(dst->data, src->data, dst->size);
}

void chunk_deinit(struct chunk * c)
{
	if (c->id >= 0)
		free(c->data);
	c->id = -1;
}

struct chunk_output * output_create(struct measures * ms, const char *config)
{
	struct chunk_output * outg = NULL;
	struct tag * tags;
	int i, len;

	outg = malloc(sizeof(struct chunk_output));
	outg->measure = ms;
	outg->buff = NULL;
	outg->out = NULL;
	outg->next_out = -1;
	outg->head = 0;

	tags = grapes_config_parse(config);
	grapes_config_value_int_default(tags, "outbuff_size", &len, 75);
	outg->buff_length = len > 0 ? len : 75;
	grapes_config_value_int_default(tags, "outbuff_reorder", &len, 1);
	outg->reorder = len ? 1 : 0;
	free(tags);


	outg->buff = (struct chunk *) malloc(outg->buff_length * sizeof(struct chunk));
	for (i=0; i<outg->buff_length; i++)
		(outg->buff)[i].id = -1;
	outg->out = out_stream_init("/dev/stdout", config);

	if (outg->out == NULL)
		output_destroy(&outg);

	return outg;
}

void output_buffer_print(struct chunk_output * co)
{
	uint16_t i;

	fprintf(stderr, "[");
	for(i=0; i<co->buff_length; i++)
		fprintf(stderr, "| %d |", co->buff[i].id);
	fprintf(stderr, "]\n");
}

void output_destroy(struct chunk_output ** outg)
{
	int i;

	if (outg && *outg)
	{
		if((*outg)->buff)
		{
			for (i=0; i<(*outg)->buff_length; i++)
				chunk_deinit(&(((*outg)->buff)[i]));
			free((*outg)->buff);
		}
		if((*outg)->out)
			out_stream_close((*outg)->out);
		free(*outg);
		*outg = NULL;
	}
}

int output_send_next(struct chunk_output * outg)
{
	int res = 0;
	if ((outg->buff)[outg->head].id >= 0)
	{
		chunk_write(outg->out, &((outg->buff)[outg->head]));
		res = 1;
		chunk_deinit(&((outg->buff)[outg->head]));
	}
	outg->next_out++;
	outg->head = (outg->head + 1) % outg->buff_length;
	return res;
}

int output_buff_flush(struct chunk_output * outg, int len)
{
	int i, res = 0;

	for(i=0; i<len; i++)
		res += output_send_next(outg);

	return res;
}

int output_deliver(struct chunk_output* outg, const struct chunk *c)
/* returns the number of of chunks sent out or -1 if chunks was too late or corrupted */
{
	int res = -1;
	int new_pos;

	if (outg && c)
	{
		res = 0;
		if (outg->reorder)
		{
			if (c->id >= outg->next_out)
			{
				if (outg->next_out < 0)	// case we have not initialized yet
					outg->next_out = c->id;

				// we make sure packet fits
				new_pos = (outg->head + c->id - outg->next_out) % outg->buff_length;
				if (c->id >= outg->next_out + outg->buff_length) // we need to free space
				{
					res += output_buff_flush(outg, c->id - outg->next_out - outg->buff_length + 1);
					outg->head = (new_pos + 1) % outg->buff_length;
				}


				// we place the packet
				if (c->id != (outg->buff[new_pos]).id)  // we do not want duplicates
					chunk_clone(&(outg->buff[new_pos]), c);

				// we flush everygthing possible
				while(outg->buff[outg->head].id >= 0)
					res += output_send_next(outg);

				// output_buffer_print(outg);
			}
			
		} else {
			chunk_write(outg->out, c);
			res++;
		}
	}

	return res;
}
