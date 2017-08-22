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

#ifndef __FRAG_REQUEST_H__
#define __FRAG_REQUEST_H__

#include<net_msg.h>
#include<fragment.h>

struct frag_request {  // extends net_msg, do not move nm parameter
	struct net_msg nm;
	frag_id_t id;
	packet_id_t pid;
};

struct frag_request * frag_request_create(const struct nodeID * from, const struct nodeID * to, packet_id_t pid, frag_id_t fid, struct list_head * list);

void frag_request_destroy(struct frag_request ** fr);

struct list_head * frag_request_list_element(struct frag_request *f);

ssize_t frag_request_send(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct frag_request * fr, uint8_t * buff, size_t buff_len);

struct frag_request * frag_request_decode(const struct nodeID *dst, const struct nodeID *src, const uint8_t * buff, size_t buff_len);

int8_t frag_request_encode(struct frag_request *fr, uint8_t * buff, size_t buff_len);

#endif
