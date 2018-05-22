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

#ifndef __NETWORK_SHAPER_H__
#define __NETWORK_SHAPER_H__

#include<network_manager.h>
#include<stdint.h>
#include<sys/time.h>

#define DEFAULT_BYTERATE 1000000
#define DEFAULT_BYTERATE_MULTIPLYER 8

struct network_shaper;

struct network_shaper * network_shaper_create(const char * config);

void network_shaper_destroy(struct network_shaper ** ns);

int8_t network_shaper_next_sending_interval(struct network_shaper * ns, struct timeval * interval);

int8_t network_shaper_register_sent_bytes(struct network_shaper * ns, size_t data_size);

int8_t network_shaper_update_bitrate(struct network_shaper * ns, size_t data_size);

#endif
