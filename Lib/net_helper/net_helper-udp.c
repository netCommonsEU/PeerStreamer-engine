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

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <net_helpers.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#else
#define _WIN32_WINNT 0x0501 /* WINNT>=0x501 (WindowsXP) for supporting getaddrinfo/freeaddrinfo.*/
#include "win32-net.h"
#endif

#include "net_helper.h"

struct nodeID {
	struct sockaddr_storage addr;
	uint16_t occurrences;
	int fd;
};

int wait4data(const struct nodeID *s, struct timeval *tout, int *user_fds)
/* returns 0 if timeout expires 
 * returns -1 in case of error of the select function
 * retruns 1 if the nodeID file descriptor is ready to be read
 * 					(i.e., some data is ready from the network socket)
 * returns 2 if some of the user_fds file descriptors is ready
 */
{
	fd_set fds;
	int i, res, max_fd;

	FD_ZERO(&fds);
	if (s && s->fd >= 0) {
		max_fd = s->fd;
		FD_SET(s->fd, &fds);
	} else {
		max_fd = -1;
	}
	if (user_fds) {
		for (i = 0; user_fds[i] != -1; i++) {
			FD_SET(user_fds[i], &fds);
			if (user_fds[i] > max_fd) {
				max_fd = user_fds[i];
			}
		}
	}
	res = select(max_fd + 1, &fds, NULL, NULL, tout);
	if (res <= 0) {
		return res;
	}
	if (s && FD_ISSET(s->fd, &fds)) {
		return 1;
	}

	/* If execution arrives here, user_fds cannot be 0
	(an FD is ready, and it's not s->fd) */
	for (i = 0; user_fds[i] != -1; i++) {
		if (!FD_ISSET(user_fds[i], &fds)) {
			user_fds[i] = -2;
		}
	}

	return 2;
}

int register_network_fds(const struct nodeID *s, fd_register_f func, void *handler)
{
	if (s) 
		func(handler, s->fd, 'r');
	return 0;
}

struct nodeID *create_node(const char *IPaddr, int port)
{
	struct nodeID *s = NULL;
	int error = 0;
	struct addrinfo hints, *result = NULL;

	if (IPaddr && port >= 0)
	{
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;

		s = malloc(sizeof(struct nodeID));
		memset(s, 0, sizeof(struct nodeID));
		s->occurrences = 1;
		s->fd = -1;

		if ((error = getaddrinfo(IPaddr, NULL, &hints, &result)) == 0)
		{
			s->addr.ss_family = result->ai_family;
			switch (result->ai_family) {
				case (AF_INET):
					((struct sockaddr_in *)&s->addr)->sin_port = htons(port);
					error = inet_pton (result->ai_family, IPaddr, &((struct sockaddr_in *)&s->addr)->sin_addr);
					if (error > 0)
						error = 0;
					break;
				case (AF_INET6):
					((struct sockaddr_in6 *)&s->addr)->sin6_port = htons(port);
					error = inet_pton (result->ai_family, IPaddr, &(((struct sockaddr_in6 *) &s->addr)->sin6_addr));
					if (error > 0)
						error = 0;
					break;
				default:
					fprintf(stderr, "Cannot resolve address family %d for '%s'\n", result->ai_family, IPaddr);
					error = -1;
			} 
		}
	}
	if (error)
	{
		fprintf(stderr, "Cannot resolve hostname '%s'\n", IPaddr);
		nodeid_free(s);
		s = NULL;
	}
	if (result)
		freeaddrinfo(result);

	return s;
}

struct nodeID *net_helper_init(const char *my_addr, int port, const char *config)
{
	int res;
	struct nodeID *myself = NULL;
	struct sockaddr_in bind_addr;
	struct sockaddr_in6 bind_addr6;
	socklen_t addr_len;

	if (my_addr && port >= 0)
	{
		myself = create_node(my_addr, port);
		if (myself)
			myself->fd =  socket(myself->addr.ss_family, SOCK_DGRAM, 0);
		if (myself && myself->fd >= 0)
			switch (myself->addr.ss_family)
			{
				case (AF_INET):
					addr_len = sizeof(struct sockaddr_in);
					memmove(&bind_addr, &(myself->addr), addr_len);
					bind_addr.sin_addr.s_addr = htonl(INADDR_ANY);
					res = bind(myself->fd, (struct sockaddr *)&bind_addr, addr_len);
					getsockname(myself->fd, (struct sockaddr *)&bind_addr, &addr_len);
					((struct sockaddr_in*)&(myself->addr))->sin_port = bind_addr.sin_port;
					break;
				case (AF_INET6):
					addr_len = sizeof(struct sockaddr_in6);
					memmove(&bind_addr6, &(myself->addr), addr_len);
					bind_addr6.sin6_addr = in6addr_any;
					res = bind(myself->fd, (struct sockaddr *)&bind_addr6, addr_len);
					getsockname(myself->fd, (struct sockaddr *)&bind_addr6, &addr_len);
					((struct sockaddr_in6*)&(myself->addr))->sin6_port = bind_addr6.sin6_port;
					break;
				default:
					fprintf(stderr, "Cannot resolve address family %d in bind\n", myself->addr.ss_family);
					res = -1;
					break;
			}
		if (myself && (myself->fd < 0 || res < 0))
		{
			nodeid_free(myself);
			myself = NULL;
		}

	}
	return myself;
}

void bind_msg_type (uint8_t msgtype)
{
}

int send_to_peer(const struct nodeID *from, const struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
	return sendto(from->fd, buffer_ptr, buffer_size, MSG_CONFIRM, (const struct sockaddr *)&(to->addr), sizeof(struct sockaddr_storage));
}

int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
	struct nodeID * node;
	int res;
	socklen_t len;

	node = malloc(sizeof(struct nodeID));
	memset(node, 0, sizeof(struct nodeID));
	node->occurrences = 1;
	node->fd = -1;
	len = sizeof(struct sockaddr_storage);

	res = recvfrom(local->fd, buffer_ptr, buffer_size, 0, (struct sockaddr *)&(node->addr), &len);

	if (res <=0 )
	{
		nodeid_free(node);
		node = NULL;
	}
	*remote = node;

	return res;
}

int node_addr(const struct nodeID *s, char *addr, int len)
{
	int n = -1;

	if (addr && len > 0)
	{
		if (s)
		{
			n = nodeid_dump((uint8_t *) addr, s, len);
			if (n>0)
				addr[n-1] = '\0';
		} else
			n = snprintf(addr, len , "None");
	}
	return n;
}

struct nodeID *nodeid_dup(const struct nodeID *s)
{
	struct nodeID * n;

	n = (struct nodeID *) s;
	if (n)
		n->occurrences++;
  return n;
}

int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
	if (s1 && s2)
		return (nodeid_cmp(s1, s2) == 0) ? 1 : 0;
	return 0;
}

int nodeid_cmp(const struct nodeID *s1, const struct nodeID *s2)
{
	char ip1[INET6_ADDRSTRLEN], ip2[INET6_ADDRSTRLEN];
	int res = 0;

	if (s1 && s2 && (s1 != s2))
	{
		node_ip(s1, ip1, INET6_ADDRSTRLEN);
		node_ip(s2, ip2, INET6_ADDRSTRLEN);
		res = strcmp(ip1, ip2);
		if (res == 0)
			res = node_port(s1) - node_port(s2);
	} else {
		if (s1 && !s2)
			res = 1;
		if (s2 && !s1)
			res = -1;
	}
	return res;
}

int nodeid_dump(uint8_t *b, const struct nodeID *s, size_t max_write_size)
{
	char ip[INET6_ADDRSTRLEN];
	int port;
	int res = -1;

	if (s && b)
	{
		node_ip(s, ip, INET6_ADDRSTRLEN);
		port = node_port(s);
		if (max_write_size >= strlen(ip) + 1 + 5)
			res = sprintf((char *)b, "%s:%d-", ip, port);
	}
	return res;
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
	char * ptr;
	char * socket;
	int port;
	struct nodeID *res = NULL;

	if (b && len)
	{
		ptr = strchr((const char *) b, '-');
		*len = ptr-(char *)b + 1;
		socket = malloc(sizeof(char) * (*len));
		memmove(socket, b, sizeof(char) * (*len));
		socket[(*len)-1] = '\0';

		ptr = strrchr(socket, ':');
		port = atoi(ptr+1);

		*ptr = '\0';

		res = create_node(socket, port);
		free(socket);
	}
	return res;
}

void nodeid_free(struct nodeID *s)
{
	if (s)
	{
		s->occurrences--;
		if (s->occurrences == 0)
		{
			if (s->fd >= 0)
				close(s->fd);
			free(s);
		}
	}
}

int node_ip(const struct nodeID *s, char *ip, int len)
{
	const char *res = NULL;

	if (s && ip)
	{
		switch (s->addr.ss_family)
		{
			case AF_INET:
				res = inet_ntop(s->addr.ss_family, &((const struct sockaddr_in *)&s->addr)->sin_addr, ip, len);
				break;
			case AF_INET6:
				res = inet_ntop(s->addr.ss_family, &((const struct sockaddr_in6 *)&s->addr)->sin6_addr, ip, len);
				break;
		}
		if (!res && len)
			ip[0] = '\0';
	}

	return res ? 0 : -1;
}

int node_port(const struct nodeID *s)
{
	int res = -1;

	if (s)
	{
		switch (s->addr.ss_family) {
			case AF_INET:
				res = ntohs(((const struct sockaddr_in *) &s->addr)->sin_port);
				break;
			case AF_INET6:
				res = ntohs(((const struct sockaddr_in6 *)&s->addr)->sin6_port);
				break;
		}
	}
	return res;
}

void net_helper_periodic(struct nodeID *s, struct timeval * interval)
{
	if (interval)
		interval->tv_sec = 1000;
}

void net_helper_deinit(struct nodeID *s)
{
	nodeid_free(s);
}
