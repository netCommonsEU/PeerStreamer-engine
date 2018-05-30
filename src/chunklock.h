/*
 * Copyright (c) 2010-2011 Csaba Kiraly
 * Copyright (c) 2010-2011 Luca Abeni
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
#ifndef CHUNKLOCK_H
#define CHUNKLOCK_H

#include <peer.h>
#include <stdint.h>

struct chunk_locks;

struct chunk_locks * chunk_locks_create(uint32_t lock_timeout_ms);
void chunk_locks_destroy(struct chunk_locks ** cl);
void chunk_lock(struct chunk_locks * cl, int flowid, int chunkid, struct peer *from);
void chunk_unlock(struct chunk_locks * cl, int flowid, int chunkid);
int chunk_islocked(struct chunk_locks * cl, int flowid, int chunkid);

#endif //CHUNKLOCK_H
