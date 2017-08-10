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

#include<fragment.h>
#include<string.h>

int8_t fragment_init(struct fragment * f, const struct nodeID * from, const struct nodeID * to, packet_id_t pid, frag_id_t frag_num, frag_id_t id, const uint8_t * data, size_t data_size, struct list_head * list)
{
	int8_t res = -1;

	if (f && from && to)
	{
		res = net_msg_init((struct net_msg *) f, NET_FRAGMENT, from, to, list);
		if (res == 0)
		{
			f->data_size = data_size;
			if (data)
			{
				f->data = malloc(sizeof(uint8_t) * data_size);
				memmove(f->data, data, f->data_size);
			} else
				f->data = NULL;
			f->id = id;
			f->pid = pid;
			f->frag_num = frag_num;
		}
	}

	return res;
}

void fragment_deinit(struct fragment * f)
{
	if (f)
	{
		if(f->data)
			free(f->data);
		net_msg_deinit((struct net_msg *) f);
	}
}

struct list_head * fragment_list_element(struct fragment *f)
{
	if (f)
		return &((struct net_msg*)f)->list;
	return NULL;
}
