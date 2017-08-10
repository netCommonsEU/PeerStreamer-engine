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
#include<net_helper.h>

int8_t net_msg_init(struct net_msg * msg, net_msg_t type, const struct nodeID * from, const struct nodeID * to, struct list_head *list)
{
	int8_t res = -1;

	if (msg && from && to)
	{
		msg->type = type;
		msg->to = nodeid_dup(to);
		msg->from = nodeid_dup(from);
		if (list)
			list_add(&(msg->list), list);
		else
			INIT_LIST_HEAD(&(msg->list));
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

// int net_msg_send(struct net_msg * msg)
// {
// 	int res;
// 	switch (msg->type) {
// 		default:
// 			res = sendto(msg->from->fd, buffer_ptr, buffer_len, MSG_CONFIRM, (const struct sockaddr *)&(msg->to->addr), sizeof(struct sockaddr_storage));
// 	}
// }
