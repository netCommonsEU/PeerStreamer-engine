/*
 * Copyright (c) 2018 Luca Baldesi
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
#include<malloc.h>
#include<string.h>
#include<sys/time.h>
#include<dbg.h>
#include<dyn_chunk_buffer.h>
#include<rbtree/rbtree.h>

#define MIN_BUFF_SIZE 50
// CHUNK_BUFFER_TIME indicates how many seconds the chunk buffer should last
#define CHUNK_BUFFER_TIME 2
#define for_each_chunk(dcb,ch) for(i = 0, ch=(struct chunk*)&((dcb)->nodes[0]);i < (dcb)->last_free; i++, ch=(struct chunk*)&((dcb)->nodes[i]))


struct dyn_node {
	struct chunk c;  // must stay in first position
	struct rb_node time_node;  // time-based ordering
	struct rb_node index_node;  // chunk id/flow id ordering
};

struct dyn_chunk_buffer {
	uint32_t size;
	uint32_t last_free;
	struct dyn_node * nodes;
	struct rb_root time_root;
	struct rb_root index_root;
	uint64_t last_update;
	float alpha_memory;
	double estimated_chunk_rate;  // chunk per second
};

void chunk_free(struct chunk *c)
{
	if (c->data)
		free(c->data);
	if (c->attributes)
		free(c->attributes);
	c->data = NULL;
	c->attributes = NULL;
	c->id = -1;
}

int8_t dyn_node_eval_index(const struct dyn_node * dn, chunkid_t cid, flowid_t fid)
{
	const struct chunk *c;

	c = (const struct chunk *) dn;

	if (c->flow_id > fid)
		return 1;
	if (c->flow_id < fid)
		return -1;
	if (c->id > cid)
		return 1;
	if (c->id < cid)
		return -1;
	return 0;
}

struct dyn_chunk_buffer * dyn_chunk_buffer_create()
{
	struct dyn_chunk_buffer * dcb= NULL;
	struct timeval now;

	dcb = malloc(sizeof(struct dyn_chunk_buffer));
	dcb->size = MIN_BUFF_SIZE;
	dcb->last_free = 0;
	dcb->nodes = malloc(sizeof(struct dyn_node)*dcb->size);
	memset(dcb->nodes, 0, sizeof(struct dyn_node)*dcb->size);
	dcb->time_root = RB_ROOT;
	dcb->index_root = RB_ROOT;
	dcb->alpha_memory = 0.9;
	gettimeofday(&now, NULL);
	dcb->last_update = now.tv_sec * 1000000ULL + now.tv_usec;
	dcb->estimated_chunk_rate = ((float)dcb->size)/CHUNK_BUFFER_TIME;
	return dcb;
}

void dyn_chunk_buffer_destroy(struct dyn_chunk_buffer ** dcb)
{
	struct chunk * c;
	uint32_t i;

	if (dcb && *dcb)
	{
		if ((*dcb)->nodes)
		{
			for_each_chunk(*dcb,c)
				chunk_free(c);
			free((*dcb)->nodes);
		}
		free(*dcb);
		*dcb = NULL;
	}
}

struct dyn_node * dyn_chunk_buffer_oldest(struct dyn_chunk_buffer * dcb)
{
	struct rb_node *node;

	node = rb_first(&(dcb->time_root));
	return container_of(node, struct dyn_node, time_node);
}

int8_t dyn_chunk_buffer_insertable(struct dyn_chunk_buffer *dcb, struct chunk * c)
{
	int8_t res = 0;
	struct dyn_node *ptr;
	struct rb_node **new;
	int64_t delta;

	if (dcb->last_free >= dcb->size && ((struct chunk *)dyn_chunk_buffer_oldest(dcb))->timestamp > c->timestamp)
		res = -1;

	new = &(dcb->time_root.rb_node);	
	while (*new && !res)
	{
		ptr = container_of(*new, struct dyn_node, time_node);
		delta = c->timestamp - ((struct chunk *)ptr)->timestamp;
		if (delta < 0)
			new = &((*new)->rb_left);
		if (delta > 0)
			new = &((*new)->rb_right);
		if (delta == 0)
			res = -2;
	}

	new = &(dcb->index_root.rb_node);	
	while (*new && !res)
	{
		ptr = container_of(*new, struct dyn_node, index_node);
		delta = dyn_node_eval_index(ptr, c->id, c->flow_id);
		if (delta < 0)
			new = &((*new)->rb_left);
		if (delta > 0)
			new = &((*new)->rb_right);
		if (delta == 0)
			res = -3;
	}
	return res;
}

int8_t _dyn_chunk_buffer_add_chunk(struct dyn_chunk_buffer * dcb, struct chunk * c)
{
	struct dyn_node *dnode,*ptr;
	struct rb_node **new, *parent;
	int64_t delta;

	if (dcb->last_free < dcb->size)
		dnode= &(dcb->nodes[dcb->last_free++]);
	else
	{
		dnode = dyn_chunk_buffer_oldest(dcb);
		rb_erase(&(dnode->time_node), &(dcb->time_root));
		rb_erase(&(dnode->index_node), &(dcb->index_root));
		chunk_free((struct chunk *)dnode);
	}
	memmove(dnode, c, sizeof(struct chunk));	// chunk parameter copying

	new = &(dcb->time_root.rb_node);	
	parent = NULL;
	while (*new)
	{
		ptr = container_of(*new, struct dyn_node, time_node);
		delta = c->timestamp - ((struct chunk *)ptr)->timestamp;
		parent = *new;
		if (delta < 0)
			new = &((*new)->rb_left);
		if (delta > 0)
			new = &((*new)->rb_right);
	}
	rb_link_node(&dnode->time_node, parent, new);
	rb_insert_color(&dnode->time_node, &dcb->time_root);

	new = &(dcb->index_root.rb_node);	
	parent = NULL;
	while (*new)
	{
		ptr = container_of(*new, struct dyn_node, index_node);
		delta = dyn_node_eval_index(ptr, c->id, c->flow_id);
		parent = *new;
		if (delta < 0)
			new = &((*new)->rb_left);
		if (delta > 0)
			new = &((*new)->rb_right);
	}
	rb_link_node(&dnode->index_node, parent, new);
	rb_insert_color(&dnode->index_node, &dcb->index_root);

	return 0;
}

void dyn_chunk_buffer_reshape(struct dyn_chunk_buffer *dcb, uint32_t size)
{
	struct dyn_node * nodes, *dnode;
	struct rb_root time_root;
	struct rb_node * node;
	uint32_t cnt = 0;

	dprintf("[DEBUG] Triggered reshaping of chunk buffer!, size %d\n", size);
	time_root = dcb->time_root;
	nodes = dcb->nodes;

	dcb->size = size;
	dcb->nodes = malloc(sizeof(struct dyn_node)*dcb->size);
	memset(dcb->nodes, 0, sizeof(struct dyn_node)*dcb->size);
	dcb->time_root = RB_ROOT;
	dcb->index_root = RB_ROOT;
	dcb->last_free = 0;

	node = rb_last(&time_root);
	while (node && cnt < size)
	{
			dnode = container_of(node, struct dyn_node, time_node);
			if (cnt < dcb->size)
				_dyn_chunk_buffer_add_chunk(dcb, (struct chunk *) dnode);
			else 
				chunk_free((struct chunk *) dnode);
			cnt++;
			node = rb_prev(node);
	}

	free(nodes);
}

void dyn_chunk_buffer_update(struct dyn_chunk_buffer * dcb)
{
	uint64_t time;
	uint32_t size;
	double delta;
	struct timeval now;

	gettimeofday(&now, NULL);
	time = now.tv_sec * 1000000ULL + now.tv_usec;
	delta = ((time - dcb->last_update))/1000000.0;
	delta = delta > 0.0001 ? delta : 0.0001;
	dcb->estimated_chunk_rate = (dcb->alpha_memory * dcb->estimated_chunk_rate)+(1-dcb->alpha_memory)/delta;
	dcb->last_update = time;

	size = CHUNK_BUFFER_TIME*dcb->estimated_chunk_rate;
	size = size < MIN_BUFF_SIZE ? MIN_BUFF_SIZE : size;
	if (size >= (dcb->size*2))
		dyn_chunk_buffer_reshape(dcb, (dcb->size)*2);
	else if (size <= (((float)dcb->size)/4))
		dyn_chunk_buffer_reshape(dcb, ((float)dcb->size)/2);
}

int8_t dyn_chunk_buffer_add_chunk(struct dyn_chunk_buffer * dcb, struct chunk * c)
{
	int8_t res=-1;
	if (dcb && c && !dyn_chunk_buffer_insertable(dcb, c))
	{
		dyn_chunk_buffer_update(dcb);
		return _dyn_chunk_buffer_add_chunk(dcb, c);
	}
	return res;
}

const struct chunk * dyn_chunk_buffer_get_chunk(const struct dyn_chunk_buffer * dcb, chunkid_t cid, flowid_t fid)
{
	struct rb_node *node;
	struct dyn_node *dnode;
	int8_t res;
	struct chunk * c = NULL;

	if (dcb)
	{
		node = (dcb->index_root).rb_node;
		while(node && !c)
		{
			dnode = container_of(node, struct dyn_node, index_node);
			res = dyn_node_eval_index(dnode, cid, fid);
			if (res < 0)
				node = node->rb_left;
			if (res > 0)
				node = node->rb_right;
			if (res == 0)
				c = (struct chunk *)dnode;
		}
	}
	return c;
}

struct chunkID_multiSet * dyn_chunk_buffer_to_multiset(const struct dyn_chunk_buffer * dcb)
{
	struct chunk * c;
	struct chunkID_multiSet * set = NULL;
	uint32_t i;

	if(dcb)
	{
		set = chunkID_multiSet_init(0, 0);
		for_each_chunk(dcb, c)
			chunkID_multiSet_add_chunk(set, c->id, c->flow_id);
	}
	return set;
}

struct sched_chunkID * dyn_chunk_buffer_to_idarray(const struct dyn_chunk_buffer * dcb, uint32_t * len)
{
	struct sched_chunkID * ids = NULL;
	uint32_t i;
	struct chunk * c;

	if (len)
	{
		*len = 0;
		if (dcb && (*len = dcb->last_free))
		{
			ids = malloc(sizeof(struct sched_chunkID)*(*len));
			for_each_chunk(dcb, c)
			{
				ids[i].chunk_id = c->id;
				ids[i].flow_id = c->flow_id;
				ids[i].timestamp = c->timestamp;
			}
		}
	}
	return ids;
}

uint32_t dyn_chunk_buffer_length(const struct dyn_chunk_buffer *dcb)
{
	return dcb ? dcb->size : 0;
}
