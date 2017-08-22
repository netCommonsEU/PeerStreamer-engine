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

#ifndef __NET_MSG_H__
#define __NET_MSG_H__ 

#include<list.h>
#include<stdint.h>
#include<stdlib.h>
#include<sys/socket.h>

/* This module is responsible of dumping/undumping the network packets */

typedef enum net_msg_types {NET_FRAGMENT, NET_FRAGMENT_REQ} net_msg_t;

struct net_msg {
	net_msg_t type;
	struct nodeID * from;
	struct nodeID * to;
	struct list_head list;
};

int8_t net_msg_init(struct net_msg * msg, net_msg_t type, const struct nodeID * from, const struct nodeID * to, struct list_head *list);

void net_msg_deinit(struct net_msg * msg);

ssize_t net_msg_send(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct net_msg * msg, uint8_t * buff, size_t buff_len);

struct net_msg * net_msg_decode(const struct nodeID *dst, const struct nodeID *src, const uint8_t * buff, size_t buff_len);

#endif
