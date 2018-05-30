/*
 * Copyright (c) 2017 Luca Baldesi
 * Copyright (c) 2018 Massimo Girondi
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
int running = 1;
int8_t config_override = 0;
char * config;

void leave(int sig) {
	running = 0;
	fprintf(stderr, "Received signal %d, exiting!\n", sig);
}

void show_help()
{
	fprintf(stdout, "This is PStreamer, a P2P streaming platform\n");
	fprintf(stdout, "Options:\n");
	fprintf(stdout, "\t-h:\t\t\tshows this help\n");
	fprintf(stdout, "\t-c <config_str>:\tdeclares configuration CSV string for the submodules\n");
	fprintf(stdout, "\n");

	fprintf(stdout, "Configuration CSV string fields:\n");
	
        
        fprintf(stdout, "\tiface=<string>:\t\t\tnetwork interface to be used (e.g., \"lo\")\n");
	fprintf(stdout, "\tport=<int>:\t\t\tlocal port number to be used (default=6000)\n");

	fprintf(stdout, "\tbs_addr=<str>:\t\t\taddress of the root node (if any, otherwise we are the root node)\n");
	fprintf(stdout, "\tbs_port=<int>:\t\t\t port of the root node (if any, default=6000)\n");

	fprintf(stdout, "\tflow_id=<int>:\t\tflow_id of this peer (default=random)\n");
	fprintf(stdout, "\toutbuff_size=<int>:\t\tsize in chunks for the output buffer (default=75)\n");
	fprintf(stdout, "\tchunkbuffer_size=<int>:\t\tsize in chunks for the trading buffer (default=50)\n");
	fprintf(stdout, "\tsource_multiplicity=<int>:\tnumber of chunks the source pushes in seeding (default=3)\n");
	fprintf(stdout, "\tfilename=<string>:\t\tfilename of a media content to be streamed (source side only)\n");
	fprintf(stdout, "\tAF=INET|INET6:\t\t\taddress family, IPv4 or IPv6 (default=INET)\n");
	fprintf(stdout, "\toffer_per_period=<int>:\t\tamount of offer to perform per offer period (default=1)\n");
	fprintf(stdout, "\tneighbourhood_size=<int>:\ttarget neighbourhood size (default=30)\n");
	fprintf(stdout, "\tpeer_timeout=<int>:\t\ttimeout in seconds after which a peer is considered dead (default=10)\n");
        
	fprintf(stdout, "\tpeers_per_offer=<int>:\t\tnumber of peers to offer chunks to (default=1)\n");
	fprintf(stdout, "\tchunks_per_peer_offer=<int>:\t\tmax number of chunks to be sent to a peer (default=1)\n");
	fprintf(stdout, "\tpeer_timeout=<int>:\t\ttimeout in seconds after which a peer is considered dead (default=10)\n");
	fprintf(stdout, "\tdist_type=random|turbo:\t\tP2P distribution policy (default=random)\n");
}

void cmdline_parse(int argc, char *argv[])
{
	int o;
	while ((o = getopt(argc, argv, "c:h")) != -1) {
		switch(o) {
			case 'c':
				config = strdup(optarg);
				config_override = 1;
				break;
			case 'h':
				show_help();
				running = 0;
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

	ps = psinstance_create(config);
	while (ps && running)
		psinstance_poll(ps, 5000000);

	if (config_override)
		free(config);
	psinstance_destroy(&ps);
	return 0;
}
