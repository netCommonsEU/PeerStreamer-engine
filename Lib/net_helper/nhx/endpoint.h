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

#ifndef __ENDPOING_H__
#define __ENDPOING_H__

#include<net_helper.h>
#include<list.h>
#include<stdint.h>
#include<stdlib.h>
#include<fragmented_packet.h>


struct endpoint;

struct endpoint * endpoint_create(const struct nodeID * node, size_t frag_size, uint16_t max_pkt_age);

void endpoint_destroy(struct endpoint ** e);

int8_t endpoint_cmp(const void * e1, const void *e2);

struct list_head * endpoint_enqueue_outgoing_packet(struct endpoint * e, const struct nodeID * src, const uint8_t * data, size_t data_len);

packet_state_t endpoint_add_incoming_fragment(struct endpoint * e, const struct fragment *f, struct list_head * requests);

int8_t endpoint_pop_incoming_packet(struct endpoint *e, packet_id_t pid, uint8_t * buff, size_t * size);

struct fragment * endpoint_get_outgoing_fragment(struct endpoint *e, packet_id_t pid, frag_id_t fid);

#endif
