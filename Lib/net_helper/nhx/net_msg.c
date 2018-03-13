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

#include<net_msg.h>
#include<fragment.h>
#include<frag_request.h>
#include<net_helper.h>
#include<string.h>

int8_t net_msg_init(struct net_msg * msg, net_msg_t type, const struct nodeID * from, const struct nodeID * to, struct list_head *list)
{
	int8_t res = -1;

	if (msg && from && to)
	{
		msg->type = type;
		msg->to = nodeid_dup(to);
		msg->from = nodeid_dup(from);
		if (list)
			list_add_tail(&(msg->list), list);
		else
			memset(&(msg->list), 0, sizeof(struct list_head));

		res = 0;

	}
	return res;
}

void net_msg_deinit(struct net_msg * msg)
{
	list_del(&(msg->list));
	nodeid_free(msg->from);
	nodeid_free(msg->to);
	msg->from = NULL;
	msg->to = NULL;
}

ssize_t net_msg_send(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct net_msg * msg, uint8_t * buff, size_t buff_len)
{
 	switch (msg->type) {
		case NET_FRAGMENT:
			return fragment_send(sockfd, dest_addr, addrlen, (struct fragment*) msg, buff, buff_len);
		case NET_FRAGMENT_REQ:
			return frag_request_send(sockfd, dest_addr, addrlen, (struct frag_request*) msg, buff, buff_len);
 		default:
 			return -1; 
 	}
}

struct net_msg * net_msg_decode(const struct nodeID *dst, const struct nodeID *src, const uint8_t * buff, size_t buff_len)
{
 	switch (*((net_msg_t*)buff)) {
		case NET_FRAGMENT:
			return (struct net_msg*) fragment_decode(dst, src, buff, buff_len);
		case NET_FRAGMENT_REQ:
			return (struct net_msg*) frag_request_decode(dst, src, buff, buff_len);
 		default:
 			return NULL;
 	}
}
