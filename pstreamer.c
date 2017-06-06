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

#include<psinstance.h>
#include<malloc.h>
#include<signal.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<net_helper.h>
#include<sys/time.h>

int srv_port = 7000;
char * srv_ip = "127.0.0.1";
char * config = "iface=lo";
int running = 1;
int8_t ip_override = 0, config_override = 0;

void leave(int sig) {
	running = 0;
	fprintf(stderr, "Received signal %d, exiting!\n", sig);
}

void cmdline_parse(int argc, char *argv[])
{
	int o;
	while ((o = getopt(argc, argv, "p:i:c:")) != -1) {
		switch(o) {
			case 'p':
				srv_port = atoi(optarg);
				break;
			case 'i':
				srv_ip = strdup(optarg);
				ip_override = 1;
				break;
			case 'c':
				config = strdup(optarg);
				config_override = 1;
				break;
			default:
				fprintf(stderr, "Error: unknown option %c\n", o);
				exit(-1);
		}
	}
}


int main(int argc, char **argv)
{
	struct psinstance * ps;

	(void) signal(SIGTERM, leave);
	(void) signal(SIGINT, leave);
	cmdline_parse(argc, argv);

	ps = psinstance_create(srv_ip, srv_port, config);
	while (ps && running)
		psinstance_poll(ps, 50000000);

	if (config_override)
		free(config);
	if (ip_override)
		free(srv_ip);
	psinstance_destroy(&ps);
	return 0;
}
