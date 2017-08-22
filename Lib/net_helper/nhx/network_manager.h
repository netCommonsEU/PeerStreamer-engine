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

#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include<stdint.h>
#include<stdlib.h>
#include<net_helper.h>
#include<fragmented_packet.h>
#include<fragment.h>

struct network_manager;

struct network_manager * network_manager_create(const char * config);

void network_manager_destroy(struct network_manager ** nm);

/***************************Ougoing*********************************/
int8_t network_manager_enqueue_outgoing_packet(struct network_manager *nm, const struct nodeID *src, const struct nodeID * dst, const uint8_t * data, size_t data_len);

struct net_msg * network_manager_pop_outgoing_net_msg(struct network_manager *nm);

int8_t network_manager_outgoing_queue_ready(struct network_manager *nm);

/************************Incoming*************************************/

packet_state_t network_manager_add_incoming_fragment(struct network_manager * nm, const struct fragment * f);

int8_t network_manager_pop_incoming_packet(struct network_manager *nm, const struct nodeID * src, packet_id_t id, uint8_t * buff, size_t *size);

/*************************PolicyDriven***********************************/

int8_t network_manager_enqueue_outgoing_fragment(struct network_manager *nm, const struct nodeID * dst, packet_id_t id, frag_id_t fid);

#endif
