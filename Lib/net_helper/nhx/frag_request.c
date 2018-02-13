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
#include<int_coding.h>

#define FRAG_REQUEST_HEADER_LEN (sizeof(net_msg_t) + sizeof(packet_id_t) + sizeof(frag_id_t))

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

int8_t frag_request_encode(struct frag_request *fr, uint8_t * buff, size_t buff_len)
{
	int8_t res = -1;
	uint8_t * ptr;

	ptr = buff;
	if (fr && buff && buff_len >= FRAG_REQUEST_HEADER_LEN)
	{
		*((net_msg_t*) ptr) = NET_FRAGMENT_REQ;
		ptr += 1;
		int16_cpy(ptr, fr->pid);
		ptr += 2;
		int16_cpy(ptr, fr->id);
		ptr += 2;
		
		res = 0;
	}
	return res;
}

ssize_t frag_request_send(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct frag_request * fr, uint8_t * buff, size_t buff_len)
{
	ssize_t res = -1;

	if (dest_addr && fr && buff && buff_len >= FRAG_REQUEST_HEADER_LEN)
	{
		frag_request_encode(fr, buff, buff_len);
		
		res = sendto(sockfd, buff, FRAG_REQUEST_HEADER_LEN, MSG_CONFIRM, dest_addr, addrlen);
	}
	return res;
}

struct frag_request * frag_request_decode(const struct nodeID *dst, const struct nodeID *src, const uint8_t * buff, size_t buff_len)
{
	struct frag_request * msg;
	const uint8_t * ptr;
	packet_id_t pid;
	frag_id_t fid;

	if (dst && src && buff && buff_len >= FRAG_REQUEST_HEADER_LEN)
	{
		ptr = buff + 1;
		pid = int16_rcpy(ptr);
		ptr = ptr + 2;
		fid = int16_rcpy(ptr);
		msg = frag_request_create(src, dst, pid, fid, NULL);
	}

	return msg;
}
