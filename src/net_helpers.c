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

#include <sys/types.h>
#ifndef _WIN32
#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>     /* For struct ifreq */
#include <netdb.h>
#include <dbg.h>
#else
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501 /* WINNT>=0x501 (WindowsXP) for supporting getaddrinfo/freeaddrinfo.*/
#endif
#include <ws2tcpip.h>
#endif
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "net_helpers.h"

char *iface_addr(const char *iface, enum L3PROTOCOL l3)
{
#ifndef _WIN32
    struct ifaddrs *if_addr, *ifap;
	int family;
	char *host_addr;
	int ifcount;

	if (getifaddrs(&if_addr) == -1)
	{
	  perror("getif_addrs");
	  return NULL;
	}
	ifcount = 0;
	for (ifap = if_addr; ifap != NULL; ifap = ifap->ifa_next)
	{
		if (ifap->ifa_addr == NULL)
		{
			ifcount++;
			continue;
		}
		family = ifap->ifa_addr->sa_family;
		if (l3 == IP4 && family == AF_INET && !strcmp (ifap->ifa_name, iface))
		{
			host_addr = malloc((size_t)INET_ADDRSTRLEN);
			if (!host_addr)
			{
				perror("malloc host_addr");
				return NULL;
			}
			if (!inet_ntop(AF_INET, (void *)&(((struct sockaddr_in *)(ifap->ifa_addr))->sin_addr), host_addr, INET_ADDRSTRLEN))
			{
				free(host_addr);
				perror("inet_ntop");
				return NULL;
			}
			break;
		}
		if (l3 == IP6 && family == AF_INET6 && !strcmp (ifap->ifa_name, iface))
		{
			host_addr = malloc((size_t)INET6_ADDRSTRLEN);
			if (!host_addr)
			{
				perror("malloc host_addr");
				return NULL;
			}
			if (!inet_ntop(AF_INET6, (void *)&(((struct sockaddr_in6 *)(ifap->ifa_addr))->sin6_addr), host_addr, INET6_ADDRSTRLEN))
			{
				free(host_addr);
				perror("inet_ntop");
				return NULL;
			}
			break;
		}

	}
	freeifaddrs(if_addr);
	return host_addr;
#else
    char *res;
    if(iface != NULL && strcmp(iface, "lo") == 0)
    {
	  switch (l3)
	  {
		case IP4:
			res = malloc (INET_ADDRSTRLEN);
			strcpy(res, "127.0.0.1");
		  break;
		case IP6:
			res = malloc (INET6_ADDRSTRLEN);
			strcpy(res, "::1");
		  break;
		default:
		  return NULL;
		  break;
	  }
      return res;
    }
    if(iface != NULL && inet_addr(iface) != INADDR_NONE) return strdup(iface);
    return default_ip_addr();
#endif
}

const char *hostname_ip_addr()
{
#ifndef _WIN32
  const char *ip;
  char hostname[256];
  struct addrinfo * result;
  struct addrinfo * res;
  int error;

  if (gethostname(hostname, sizeof hostname) < 0) {
    dtprintf("can't get hostname\n");
    return NULL;
  }
  dtprintf("hostname is: %s ...", hostname);

  /* resolve the domain name into a list of addresses */
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0) {
    dtprintf("can't resolve IP: %s\n", gai_strerror(error));
    return NULL;
  }

  /* loop over all returned results and do inverse lookup */
  for (res = result; res != NULL; res = res->ai_next) {
    ip = inet_ntoa(((struct sockaddr_in*)res->ai_addr)->sin_addr);
    dtprintf("IP is: %s ...", ip);
    if ( strncmp("127.", ip, 4) == 0) {
      dtprintf(":( ...");
      ip = NULL;
    } else {
      break;
    }
  }
  freeaddrinfo(result);

  return ip;
#else
  const char *ip;
  char hostname[256];
  struct addrinfo hints, *result, *res;
  int error;
  ip = malloc (INET6_ADDRSTRLEN);
  if (!ip)
  {
	  perror("hostname_ip_addr");
	  return NULL;
  }
  dtprintf(stderr, "Trying to guess IP ...");
  if (gethostname(hostname, sizeof hostname) < 0) {
    dtprintf(stderr, "can't get hostname\n");
    return NULL;
  }
  memset(&hints, 0, sizeof(hints));
  hints.ai_flags = AI_CANONNAME;
  hints.ai_family = AF_UNSPEC;
  hints.ai_protocol = IPPROTO_TCP;

  dtprintf(stderr, "hostname is: %s ...\n", hostname);
  error = getaddrinfo(hostname, NULL, NULL, &result);
  if (error != 0)
  {
    dtprintf(stderr, "can't resolve IP: %s\n", gai_strerror(error));
    return NULL;
  }

  for (res = result; res != NULL ; res = res->ai_next)
  {
      dtprintf(stderr, "Address Family is %d...", res->ai_family);
	  if ( (res->ai_family == AF_INET && l3 == IP4) ||
			  (res->ai_family == AF_INET6 && l3 == IP6))
	  {
		  if (res->ai_family == AF_INET)
		  {
			  inet_ntop(res->ai_family, &((const struct sockaddr_in *)(res->ai_addr))->sin_addr, ip, INET6_ADDRSTRLEN);
		  }
		  else
		  {
			  inet_ntop(res->ai_family, &((const struct sockaddr_in6 *)(res->ai_addr))->sin6_addr, ip, INET6_ADDRSTRLEN);
		  }
		  dtprintf(stderr, "IP is: %s ...", ip);
		  if ( (l3 == AF_INET && strncmp("127.", ip, 4) == 0) ||
				  (l3 == AF_INET6 && strncmp("::1", ip, 3) == 0)) {
			  dtprintf(stderr, ":( ...");
			  memset (&ip, 0, INET6_ADDRSTRLEN);
		  }
		  else break;
	  }
  }
  freeaddrinfo(result);

  return ip;
#endif
}

char *autodetect_ip_address(enum L3PROTOCOL l3) {
#ifdef __linux__

	char iface[IFNAMSIZ] = "";
	char line[128] = "x";

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}
	fclose(r);

	return iface_addr(iface, l3);
#else
		return hostname_ip_addr();
#endif
}

char *default_ip_addr(enum L3PROTOCOL l3)
{
  char *ip = NULL;

  dtprintf("Trying to guess IP ...");

  ip = autodetect_ip_address(l3);

  if (!ip) {
    dtprintf("cannot detect IP!\n");
    return NULL;
  }
  dtprintf("IP is: %s ...\n", ip);

  return ip;
}

char * nodeid_static_str(const struct nodeID * id)
/* WARNING: extremely error prone (but convenient)
 * This function converts on-the-fly a nodeID to a printable string without the need
 * of allocating or freeing memory. It works with statically allocated memory in the
 * stuck so it is NOT thread safe and CANNOT be used twice in the same function call.
 */
{
	static char buff[80];
	node_addr(id, buff, 80);
	return buff;
}
