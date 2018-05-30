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

#ifndef __PEER_METADATA_H__
#define __PEER_METADATA_H__

#include<stdint.h>
#include<time.h>
#include<peer.h>
#include<chunkidms.h>

#define DEFAULT_PEER_CBSIZE 50
#define DEFAULT_PEER_NEIGH_SIZE 30

struct metadata {
  uint16_t cb_size;
  uint8_t neigh_size;
} __attribute__((packed));

struct user_data {
	struct timeval bmap_timestamp;
	struct chunkID_multiSet * bmap;
};

int8_t metadata_update(struct metadata *m, uint16_t cb_size, uint8_t neigh_size);

int8_t peer_set_metadata(struct  peer *p, const struct metadata *m);

uint16_t peer_cb_size(const struct peer *p);

uint16_t peer_neigh_size(const struct peer *p);

void peer_data_init(struct peer *p);

void peer_data_deinit(struct peer *p);

struct chunkID_multiSet * peer_bmap(struct peer *p);

struct timeval * peer_bmap_timestamp(struct peer *p);

struct timeval * peer_creation_timestamp(struct peer *p);

#endif
