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
	uint32_t last_index;
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

int8_t reg_chunk_receive(struct measures * m, struct chunk *c) 
{ 
	if (m && c)  // chunk interval estimation
	{
		switch (m->cie.state) {
			case deinit:
				m->cie.first_index = c->id;
				m->cie.last_index = c->id;
				m->cie.first_timestamp = c->timestamp;
				m->cie.state = loading;
				break;
			case loading:
				break;
			case ready:
				if (c->id > (int64_t)m->cie.last_index)
				{
					m->cie.chunk_interval = ((c->timestamp - m->cie.first_timestamp))/(c->id - m->cie.first_index);
					m->cie.last_index = c->id;
				}
				m->cie.state = ready;
				break;
		}
	}
	return 0;
}

suseconds_t chunk_interval_measure(const struct measures *m)
{
	if (m->cie.state == ready)
		return m->cie.chunk_interval;
	return DEFAULT_CHUNK_INTERVAL;
}
