/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
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
#ifndef DBG_H
#define DBG_H

#include<stdio.h>
#include<stdint.h>

#include<chunk.h>
#include<net_helper.h>
#include<topology.h>
#include<psinstance.h>
#include<chunkidms.h>
#include<chunkidms_trade.h>

int ftprintf(FILE *stream, const char *format, ...);

#ifdef DEBUG
#define dprintf(...) fprintf(stderr,__VA_ARGS__)
#define dtprintf(...) ftprintf(stderr,__VA_ARGS__)
#else
#define dprintf(...)
#define dtprintf(...)
#endif


void log_signal(const struct nodeID *fromid,const struct nodeID *toid,uint16_t trans_id, enum signaling_typeMS type,const char *flag, const struct chunkID_multiSet *ms);

void log_chunk(const struct nodeID *from,const struct nodeID *to,const struct chunk *c,const char * note);

void log_neighbourhood(const struct psinstance * ps);

void log_chunk_error(const struct nodeID *from,const struct nodeID *to,const struct chunk *c,int error);


#endif	/* DBG_H */
