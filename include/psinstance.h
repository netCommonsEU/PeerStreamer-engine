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

#ifndef __PSCONTEXT_H__ 
#define __PSCONTEXT_H__ 

#include<stdint.h>

#define MSG_BUFFSIZE (512 * 1024)
#define FDSSIZE 16

typedef long suseconds_t;

struct psinstance;

/********************High-level Interface***********************/
struct psinstance * psinstance_create(const char * config);

void psinstance_destroy(struct psinstance ** ps);

int psinstance_poll(struct psinstance *ps, suseconds_t);

/********************       Utils        ***********************/
int psinstance_ip_address(const struct psinstance *ps, char * ip, int len);

int psinstance_port(const struct psinstance *ps);

/********************Additional Interface***********************/
int8_t psinstance_send_offer(struct psinstance * ps);

int8_t psinstance_inject_chunk(struct psinstance * ps);

int8_t psinstance_handle_msg(struct psinstance * ps);

int8_t psinstance_topology_update(const struct psinstance * ps);

suseconds_t psinstance_offer_interval(const struct psinstance * ps);

suseconds_t psinstance_network_periodic(struct psinstance * ps);

#endif
