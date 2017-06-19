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

#include<measures.h>
#include<string.h>

#define DEFAULT_CHUNK_INTERVAL (1000000/25)

enum data_state {deinit, loading, ready};

struct chunk_interval_estimate {
	uint32_t first_index;
	uint64_t first_timestamp;
	suseconds_t chunk_interval;
	enum data_state state;
};

struct measures {
	char * filename;
	struct chunk_interval_estimate cie;
};

void chunk_interval_estimate_init(struct chunk_interval_estimate * cie)
{
	cie->state = deinit;
}

struct measures * measures_create(const char * filename)
{
	struct measures * m;
	m = malloc(sizeof(struct measures));
	chunk_interval_estimate_init(&(m->cie));
	return m;
}

void measures_destroy(struct measures ** m)
{
	if (m && *m)
	{
		free((*m));
	}
}

int8_t measures_add_node(struct measures * m, struct nodeID * id)
{
	return 0;
}

int8_t reg_chunk_playout(struct measures * m, int id, bool b, uint64_t timestamp)
{
	return 0;
}

int8_t reg_chunk_duplicate(struct measures * m)
{
	return 0;
}

int8_t reg_neigh_size(struct measures * m, size_t t)
{
	return 0;
}

int8_t reg_offer_accept_in(struct measures * m, uint8_t t)
{ 
	return 0;
}

int8_t reg_chunk_receive(struct measures * m, int cid, uint64_t ctimestamp, int hopcount, int8_t old, int8_t duplicate)
{ 
	if (!duplicate)  // chunk interval estimation
	{
		switch (m->cie.state) {
			case deinit:
				m->cie.first_index = cid;
				m->cie.first_timestamp = ctimestamp;
				m->cie.state = loading;
				break;
			case loading:
			case ready:
				if (cid > (int64_t)m->cie.first_index)
					m->cie.chunk_interval = ((ctimestamp - m->cie.first_timestamp))/(cid - m->cie.first_index);
				m->cie.state = ready;
				break;
		}
	}
	return 0;
}

int8_t reg_offer_accept_out(struct measures * m, uint8_t t)
{ 
	return 0;
}

int8_t reg_chunk_send(struct measures * m, int id)
{ 
	return 0;
}

int8_t timeout_reception_measure(const struct nodeID * id)
{ 
	return 0;
}

int8_t offer_accept_rtt_measure(const struct nodeID * id, uint64_t rtt)
{ 
	return 0;
}

int8_t reception_measure(const struct nodeID * id)
{ 
	return 0;
}

suseconds_t chunk_interval_measure(const struct measures *m)
{
	if (m->cie.state == ready)
		return m->cie.chunk_interval;
	return DEFAULT_CHUNK_INTERVAL;
}
