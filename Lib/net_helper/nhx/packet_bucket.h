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

#ifndef __PACKET_BUCKET_H__
#define __PACKET_BUCKET_H__

#include<stdlib.h>
#include<fragment.h>
#include<fragmented_packet.h>
#include<stdint.h>


struct packet_bucket;

struct packet_bucket * packet_bucket_create(size_t frag_size, uint16_t max_pkt_age);

void packet_bucket_destroy(struct packet_bucket ** pb);

struct list_head * packet_bucket_add_packet(struct packet_bucket * pb, const struct nodeID * src, const struct nodeID *dst, packet_id_t pid, const uint8_t *data, size_t data_len);

packet_state_t packet_bucket_add_fragment(struct packet_bucket *pb, const struct fragment *f, struct list_head * requests);

int8_t packet_bucket_pop_packet(struct packet_bucket *pb, packet_id_t pid, uint8_t * buff, size_t * size);

struct fragment * packet_bucket_get_fragment(struct packet_bucket *pb, packet_id_t pid, frag_id_t fid);

#endif
