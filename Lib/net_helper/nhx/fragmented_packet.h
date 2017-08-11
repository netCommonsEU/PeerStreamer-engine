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

#ifndef __FRAGMENTED_PACKET_H__
#define __FRAGMENTED_PACKET_H__

#include<time.h>
#include<fragment.h>
#include<net_helper.h>

enum packet_state {PKT_READY, PKT_LOADING, PKT_ERROR};
typedef enum packet_state packet_state_t;

struct fragmented_packet {
	time_t creation_timestamp;
	struct fragment * frags;
	frag_id_t frag_num;
	struct list_head list;
	packet_id_t packet_id;
};

void fragmented_packet_destroy(struct fragmented_packet **);

packet_id_t fragmented_packet_id(const struct fragmented_packet *fp);

time_t fragmented_packet_creation_timestamp(const struct fragmented_packet *fp);

struct fragmented_packet * fragmented_packet_create(packet_id_t id, const struct nodeID * from, const struct nodeID *to, const uint8_t * data, size_t data_size, size_t frag_size, struct list_head ** msgs);

struct fragmented_packet * fragmented_packet_empty(packet_id_t pid, const struct nodeID *from, const struct nodeID *to, frag_id_t num_frags);

packet_state_t fragmented_packet_write_fragment(struct fragmented_packet *fp, const struct fragment *f, struct list_head ** requests);

int8_t fragmented_packet_dump_data(struct fragmented_packet *fp, uint8_t * buff, size_t * size);

struct fragment * fragmented_packet_fragment(struct fragmented_packet *fp, frag_id_t fid);

#endif
