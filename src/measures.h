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
#include<chunk.h>

struct measures;

struct measures * measures_create(const char * filename);
int8_t measures_add_node(struct measures * m, struct nodeID * id);
void measures_destroy(struct measures ** m);

/*************Storing functions***************/

int8_t reg_chunk_receive(struct measures * m, struct chunk *c);

/*************Get functions***************/
suseconds_t chunk_interval_measure(const struct measures * m);

#endif
