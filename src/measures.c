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

struct measures {
	char * filename;
};

struct measures * measures_create(const char * filename)
{
	struct measures * m;
	m = malloc(sizeof(struct measures));
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

