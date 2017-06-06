/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2014-2017 Luca Baldesi
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
#ifndef TOPOLOGY_H
#define TOPOLOGY_H

#include <stdint.h>
#include <psinstance_internal.h>
#include <measures.h>

#define MSG_TYPE_NEIGHBOURHOOD   0x22

struct topology;

struct topology * topology_create(const struct psinstance *ps, const char *config);
void topology_destroy(struct topology **t);

int topology_node_insert(struct topology *t, struct nodeID *neighbour);
struct peerset *topology_get_neighbours(struct topology *t);
void topology_update(struct topology *t);
struct peer *nodeid_to_peer(struct topology *t, struct nodeID* id, int reg);
void topology_message_parse(struct topology *t, struct nodeID *from, const uint8_t *buff, size_t len);
void peerset_print(const struct peerset * pset,const char * name);

#endif	/* TOPOLOGY_H */
