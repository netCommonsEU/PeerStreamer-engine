/*
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
#include <grapes_config.h>

#include "output.h"
#include "dbg.h"


/*
 Use a single output module (AKA dechunkiser) for all flows.
 Put different chunks in different buffers, play the chunks in the buffer of
 the chunk to reproduce (like the others buffers don't exist).
*/

struct output_buffer {
	struct chunk *buff;
	uint16_t buff_length;
	uint16_t head;
	uint8_t  reorder;
	int next_out;
        int flowid;
};

struct chunk_output {
        struct output_stream *out;
        int buff_number;
        struct output_buffer **buffs;
        struct measures * measure;
        int reorder;
        int buff_len;
};


struct output_buffer * output_buffer_init(struct chunk_output * chout, int flowid)
{
	struct output_buffer * outb=NULL;
	int j;

        outb = malloc(sizeof(struct output_buffer));
	outb->buff = NULL;
	outb->next_out = -1;
	outb->head = 0;
	outb->flowid = flowid;
	outb->buff_length = chout->buff_len;
	outb->reorder = chout->reorder;
	outb->buff = (struct chunk *) malloc(chout->buff_len * sizeof(struct chunk));

	for (j=0; j<chout->buff_len; j++)
		(outb->buff[j]).id = -1;

	return outb;
}

struct output_buffer * get_buffer(struct chunk_output * chout, int flow_id)
{
        int i;
        struct output_buffer ** new_buffs;
        struct output_buffer *ret;
        
        for( i=0; i<chout->buff_number; i++)
                if(chout->buffs[i]->flowid==flow_id)
                        return chout->buffs[i];

        //Didn't found -> create a new buffer
        new_buffs = malloc(sizeof(struct output_buffer * ) * (chout->buff_number +1));
        if(!new_buffs)
                return NULL;

        if(chout->buffs)
        {
                memcpy(new_buffs, chout->buffs,sizeof(struct output_buffer * ) * (chout->buff_number));
                free(chout->buffs);
        }

        chout->buffs=new_buffs;
        ret=chout->buffs[chout->buff_number] = output_buffer_init(chout,flow_id);
        chout->buff_number+=1;
        dprintf("Allocated new output buffer for flow %i; total number of output buffer is %i\n",flow_id,chout->buff_number);

        return ret;
}

void chunk_clone(struct chunk * dst, const struct chunk* src)
{
	dst->id = src->id;
	dst->size = src->size;
	dst->data = malloc(dst->size);
        dst->flow_id = src->flow_id;
        memmove(dst->data, src->data, dst->size);
}

void chunk_deinit(struct chunk * c)
{
	if (c->id >= 0)
		free(c->data);
	c->id = -1;
        c->flow_id = -1;
}

struct chunk_output * output_create(struct measures * ms, const char *config)
{
	struct chunk_output * chout = NULL;
	struct tag * tags;
	int i, len;

	chout = malloc(sizeof(struct chunk_output));
	chout->measure = ms;
	chout->buffs = NULL;
	chout->out = NULL;
	chout->buff_number = 0;

	tags = grapes_config_parse(config);
	grapes_config_value_int_default(tags, "outbuff_size", &len, 75);
	chout->buff_len = len > 0 ? len : 75;
	grapes_config_value_int_default(tags, "outbuff_reorder", &len, 1);
	chout->reorder = len ? 1 : 0;
	free(tags);

	chout->out = out_stream_init("/dev/stdout", config);

	if (chout->out == NULL)
		output_destroy(&chout);

	return chout;
}


void output_buffer_print(const struct chunk_output * co)
{
	uint16_t i,j;

	if(co->buffs)
	{
		for(j=0; j<co->buff_number; j++)
		{
			fprintf(stderr, "%i [%i]: [",j, co->buffs[j]->flowid);
			for(i=0; i<co->buffs[j]->buff_length; i++)
			        fprintf(stderr, "| %d |", co->buffs[j]->buff[i].id);
			fprintf(stderr, "]\n");
		}

	}

}

void output_destroy(struct chunk_output ** outg)
{
	int i,j;

	if (outg && *outg)
	{
		for(j=0; j<(*outg)->buff_number;j++)
		{
			if(((*outg)->buffs[j])->buff)
			{
				for (i=0; i<((*outg)->buffs[j])->buff_length; i++)
					chunk_deinit(&((((*outg)->buffs[j])->buff)[i]));
				free(((*outg)->buffs[j])->buff);
			}
			free((*outg)->buffs[j]);
		}

		if((*outg)->out)
			out_stream_close((*outg)->out);
		free(*outg);
		*outg = NULL;
	}
}


//Play the next chunk
int output_send_next(struct output_stream *out, struct output_buffer * outg)
{
	int res = 0;
	if ((outg->buff)[outg->head].id >= 0)
	{
		chunk_write(out, &((outg->buff)[outg->head]));
		res = 1;
		chunk_deinit(&((outg->buff)[outg->head]));
	}
	outg->next_out++;
	outg->head = (outg->head + 1) % outg->buff_length;
	return res;
}

//Play every chunk in buffer
int output_buff_flush(struct output_stream *out,struct output_buffer * outb, int len)
{
	int i, res = 0;
	dprintf("[Output Buffer] Need to flush %i chunks\n",len);
	for(i=0; i<len; i++)
		res += output_send_next(out,outb);

	return res;
}



int output_deliver_buffer(struct output_stream *out, struct output_buffer* outb, const struct chunk *c)
/* returns the number of of chunks sent out or -1 if chunks was too late or corrupted */
{
	int res = -1;
	int new_pos;

	if (outb && c)
	{
		res = 0;
		if (outb->reorder)
		{
			if (c->id >= outb->next_out) //We didn't played it yet
			{
				if (outb->next_out < 0)	// case we have not initialized yet
					outb->next_out = c->id;

				// we make sure packet fits
				new_pos = (outb->head + c->id - outb->next_out) % outb->buff_length;
				if (c->id >= outb->next_out + outb->buff_length) // we need to free space
				{
					res += output_buff_flush(out,outb, c->id - outb->next_out - outb->buff_length + 1);
					outb->head = (new_pos + 1) % outb->buff_length;
				}


				// we place the packet
				if (c->id != (outb->buff[new_pos]).id)  // we do not want duplicates
					chunk_clone(&(outb->buff[new_pos]), c);

				// we flush everygthing possible
				while(outb->buff[outb->head].id >= 0)
					res += output_send_next(out,outb);
			}

		} else {
			//Just play the chunk, we don't care about the ordering
			chunk_write(out, c);
			res++;
		}
	}

	return res;
}


/**
  Wrapper to output_deliver_buffer, called on the correct buffer.
**/
int output_deliver(struct chunk_output* cout, const struct chunk *c)
{
	int res;
	struct output_buffer * buff=NULL;

	buff = get_buffer(cout,c->flow_id);
	if (buff)
		res = output_deliver_buffer(cout->out,buff, c);
	else{
		//Error allocating new buffer, just play the chunk
		dprintf("Error allocating a new output buffer\n");
		chunk_write(cout->out, c);
		res=1;
	}
#ifdef LOG_CHUNK
	dprintf("Delivered chunk %i [%i]\n", c->id, c->flow_id);
	output_buffer_print(cout);
#endif
	return res;
}
