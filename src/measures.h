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

#ifndef __MEASURES_H__
#define __MEASURES_H__ 

#include<stdint.h>
#include<stdbool.h>
#include<net_helper.h>
#include<psinstance.h>

struct measures;

struct measures * measures_create(const char * filename);
int8_t measures_add_node(struct measures * m, struct nodeID * id);
void measures_destroy(struct measures ** m);

/*************Storing functions***************/
int8_t reg_chunk_playout(struct measures * m, int id, bool b, uint64_t timestamp);
int8_t reg_chunk_duplicate(struct measures * m);
int8_t reg_neigh_size(struct measures * m, size_t t);
int8_t reg_offer_accept_in(struct measures * m, uint8_t t);

int8_t reg_chunk_receive(struct measures * m, int cid, uint64_t ctimestamp, int hopcount, int8_t old, int8_t duplicate);
int8_t reg_offer_accept_out(struct measures * m, uint8_t t);
int8_t reg_chunk_send(struct measures * m, int id);
int8_t offer_accept_rtt_measure(const struct nodeID * id, uint64_t rtt);

/*************Get functions***************/
int8_t reception_measure(const struct nodeID * id);
int8_t timeout_reception_measure(const struct nodeID * id);
suseconds_t chunk_interval_measure(const struct measures * m);

#endif
