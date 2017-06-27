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

#include<peer_metadata.h>
#include<malloc.h>
#include<string.h>

int8_t metadata_update(struct metadata *m, uint16_t cb_size, uint8_t neigh_size)
{
	if (m)
	{
		m->cb_size = cb_size;
		m->neigh_size = neigh_size;
		return 0;
	}
	return -1;
}

int8_t peer_set_metadata(struct  peer *p, const struct metadata *m)
{
	if (p && m)
	{
		if (!(p->metadata))
			p->metadata = malloc(sizeof(struct metadata));
		memmove(p->metadata, m, sizeof(struct metadata));
		return 0;
	}
	return -1;
}

uint16_t peer_cb_size(const struct peer *p)
{
	if (p && p->metadata)
		return ((struct metadata *)p->metadata)->cb_size;
	return DEFAULT_PEER_CBSIZE;
}

uint16_t peer_neigh_size(const struct peer *p)
{
	if (p && p->metadata)
		return ((struct metadata *)p->metadata)->neigh_size;
	return DEFAULT_PEER_NEIGH_SIZE;
}
