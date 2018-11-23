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
#ifndef __DYN_CHUNK_BUFFER__
#define __DYN_CHUNK_BUFFER__

#include<chunk.h>
#include<chunkidms.h>
#include<scheduler_common.h>

struct dyn_chunk_buffer;

struct dyn_chunk_buffer * dyn_chunk_buffer_create();

void dyn_chunk_buffer_destroy(struct dyn_chunk_buffer ** dcb);

int8_t dyn_chunk_buffer_add_chunk(struct dyn_chunk_buffer * dcb, struct chunk * c);

const struct chunk * dyn_chunk_buffer_get_chunk(const struct dyn_chunk_buffer * dcb, chunkid_t cid, flowid_t fid);

struct chunkID_multiSet * dyn_chunk_buffer_to_multiset(const struct dyn_chunk_buffer * dcb);

struct sched_chunkID * dyn_chunk_buffer_to_idarray(const struct dyn_chunk_buffer * dcb, uint32_t * len);

uint32_t dyn_chunk_buffer_length(const struct dyn_chunk_buffer *dcb);

#endif
