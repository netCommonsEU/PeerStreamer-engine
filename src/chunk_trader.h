/*
 * Copyright (c) 2018 Luca Baldesi
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
#ifndef __CHUNK_TRADER_H__
#define __CHUNK_TRADER_H__

#include <stdint.h>

#include<chunk_trader.h>
#include<psinstance_internal.h>
#include<chunk.h>
#include<peer.h>
#include<topology.h>
#include<net_helper.h>
#include<chunk_attributes.h>

#define E_CANNOT_PARSE -3
#define E_CACHE_MISS -4

struct chunk_trader;

struct chunk_trader * chunk_trader_create(const struct psinstance *ps, const char *config);

void chunk_trader_destroy(struct chunk_trader **ct);

int8_t chunk_trader_add_chunk(struct chunk_trader *ct, struct chunk *c);

/** chunk actions **/
void chunk_destroy(struct chunk **c);

int8_t chunk_trader_push_chunk(struct chunk_trader *ct, struct chunk *c, int multiplicity);

struct chunk * chunk_trader_parse_chunk(struct chunk_trader *ct, struct nodeID *from, uint8_t *buff, int len);

/** signalling actions **/
int8_t chunk_trader_send_offer(struct chunk_trader *ct);

int8_t chunk_trader_msg_parse(struct chunk_trader *ct, struct nodeID *from, uint8_t *buff, int buff_len);

int8_t chunk_trader_send_bmap(const struct chunk_trader *ct, const struct nodeID *to);

/** utils **/
suseconds_t chunk_trader_offer_interval(const struct chunk_trader *ct);

int chunk_trader_buffer_size(const struct chunk_trader *ct);

#endif
