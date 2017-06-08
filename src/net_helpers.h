/*
 * Copyright (c) 2010-2011 Luca Abeni
 * Copyright (c) 2010-2011 Csaba Kiraly
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
#ifndef NET_HELPERS_H
#define NET_HELPERS_H

#include<net_helper.h>
#include<pstreamer_event.h>

#define NODE_STR_LENGTH 80

char *iface_addr(const char *iface);
char *default_ip_addr();
char * nodeid_static_str(const struct nodeID * id);
int register_network_fds(const struct nodeID *s, fd_register_f func, void *handler);

#endif	/* NET_HELPERS_H */
