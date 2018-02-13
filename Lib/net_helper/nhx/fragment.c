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
#include<stdio.h>
#include<int_coding.h>

// #define FRAGMENT_HEADER_LEN (sizeof(net_msg_t) + sizeof(packet_id_t) + sizeof(frag_id_t) + sizeof(frag_id_t) + sizeof(size_t))
#define FRAGMENT_HEADER_LEN (1 + 2 + 2 + 2 + 4)

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

int8_t fragment_encode(struct fragment * frag, uint8_t * buff, size_t buff_len)
{
	int8_t res = -1;
	uint8_t * ptr;

	ptr = buff;
	if (frag && buff && buff_len >= FRAGMENT_HEADER_LEN + frag->data_size)
	{
		*((net_msg_t*) ptr) = NET_FRAGMENT;
		ptr += 1;
		int16_cpy(ptr, frag->pid);
		ptr += 2;
		int16_cpy(ptr, frag->frag_num);
		ptr += 2;
		int16_cpy(ptr, frag->id);
		ptr += 2;
		int_cpy(ptr, frag->data_size);
		ptr += 4;
		memmove(ptr, frag->data, frag->data_size);

		res = 0;
	}
	return res;

}

ssize_t fragment_send(int sockfd, const struct sockaddr *dest_addr, socklen_t addrlen, struct fragment * frag, uint8_t * buff, size_t buff_len)
{
	ssize_t res = -1;
	ssize_t msg_len;

		
	if (dest_addr && frag && buff && buff_len >= FRAGMENT_HEADER_LEN + frag->data_size)
	{
		msg_len = FRAGMENT_HEADER_LEN + frag->data_size;
		fragment_encode(frag, buff, buff_len);
		
		res = sendto(sockfd, buff, msg_len, MSG_CONFIRM, dest_addr, addrlen);
	}
	return res;
}

struct fragment * fragment_decode(const struct nodeID *dst, const struct nodeID *src, const uint8_t * buff, size_t buff_len)
{
	struct fragment * msg = NULL;
	const uint8_t * ptr;
	packet_id_t pid;
	frag_id_t fid, frag_num;
	size_t data_len;

	if (dst && src && buff && buff_len >= FRAGMENT_HEADER_LEN)
	{
		ptr = buff + 1;
		pid = int16_rcpy(ptr);
		ptr = ptr + 2;
		frag_num = int16_rcpy(ptr);
		ptr = ptr + 2;
		fid = int16_rcpy(ptr);
		ptr = ptr + 2;
		data_len = int_rcpy(ptr);
		ptr = ptr + 4;

		if (buff_len >= FRAGMENT_HEADER_LEN + data_len)
		{
			msg = malloc(sizeof(struct fragment));
			fragment_init(msg, src, dst, pid, frag_num, fid, ptr, data_len, NULL);
		}
	}

	return msg;
}
