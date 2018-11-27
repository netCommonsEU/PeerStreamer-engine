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
#include<stdio.h>

#define DEFAULT_CHUNK_INTERVAL (1000000/25)

enum data_state {init, ready};

struct chunk_interval_estimate {
	uint64_t last_timestamp;
	float alpha;
	suseconds_t chunk_interval;
	enum data_state state;
};

struct measures {
	char * filename;
	struct chunk_interval_estimate cie;
};

void chunk_interval_estimate_init(struct chunk_interval_estimate * cie)
{
	cie->state = init;
	cie->chunk_interval = DEFAULT_CHUNK_INTERVAL;
	cie->alpha = 0.9;
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

int8_t reg_chunk_send(struct measures * m, struct chunk *c) 
{
	return reg_chunk_receive(m, c);
}

int8_t reg_chunk_receive(struct measures * m, struct chunk *c) 
{ 
	uint64_t timestamp;
	struct timeval now;
	if (m && c)  // chunk interval estimation
	{
		gettimeofday(&now, NULL);
		timestamp = now.tv_sec * 1000000ULL + now.tv_usec;
		switch (m->cie.state) {
			case init:
				m->cie.state = ready;
				break;
			case ready:
				m->cie.chunk_interval = (m->cie.alpha * m->cie.chunk_interval) + (1 - m->cie.alpha)*(timestamp - m->cie.last_timestamp);
				break;
		}
		m->cie.last_timestamp = timestamp;
	}
	return 0;
}

suseconds_t chunk_interval_measure(const struct measures *m)
{
	return m->cie.chunk_interval;
}
