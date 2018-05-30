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

#ifndef __PSCONTEXT_INTERNAL_H__
#define __PSCONTEXT_INTERNAL_H__

#include<stdint.h>
#include<net_helper.h>

struct psinstance;

struct nodeID * psinstance_nodeid(const struct psinstance * ps);

struct topology * psinstance_topology(const struct psinstance * ps);

struct measures * psinstance_measures(const struct psinstance * ps);

struct chunk_output * psinstance_output(const struct psinstance * ps);

const struct chunk_trader * psinstance_trader(const struct psinstance * ps);


#endif
