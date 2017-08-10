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
#include<frag_request.h>
#include<string.h>

struct frag_request * frag_request_create(const struct nodeID * from, const struct nodeID * to, packet_id_t pid, frag_id_t fid, struct list_head * list)
{
	struct frag_request * fr;

	fr = malloc(sizeof(struct frag_request));
	net_msg_init((struct net_msg *) fr, NET_FRAGMENT_REQ, from, to, list);
	fr->pid = pid;
	fr->id = fid;

	return fr;
}

void frag_request_destroy(struct frag_request ** fr)
{
	if (fr && *fr)
	{
		net_msg_deinit((struct net_msg *)*fr);
		free(*fr);
		*fr = NULL;
	}
}

struct list_head * frag_request_list_element(struct frag_request *f)
{
	if (f)
		return &((struct net_msg*)f)->list;
	return NULL;
}
