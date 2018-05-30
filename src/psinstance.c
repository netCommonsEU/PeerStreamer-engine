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
#include<psinstance_internal.h>
#include<grapes_config.h>
#include<malloc.h>
#include<string.h>
#include<net_helpers.h>
#include<net_helper.h>
#include<output.h>
#include<input.h>
#include<chunk_trader.h>
#include<measures.h>
#include<topology.h>
#include<grapes_msg_types.h>
#include<dbg.h>
#include<streaming_timers.h>
#include<pstreamer_event.h>

struct psinstance {
	struct nodeID * my_sock;
	struct chunk_output * chunk_out;
	struct measures * measure;
	struct topology * topology;
	struct chunk_trader * trader;
	struct input_context inc;
	struct input_desc * input;
	struct streaming_timers timers;
	char * iface;
	int port;
	suseconds_t chunk_time_interval; // microseconds
	suseconds_t chunk_offer_interval; // microseconds
	int source_multiplicity;
	enum L3PROTOCOL l3;

        int bs_port;
        char *bs_addr;

};

int config_parse(struct psinstance * ps, const char * config)
{
	struct tag * tags;
	const char *tmp_str;

	tags = grapes_config_parse(config);

	tmp_str = grapes_config_value_str_default(tags, "iface", NULL);
	ps->iface = tmp_str ? strdup(tmp_str) : NULL;
	grapes_config_value_int_default(tags, "port", &(ps->port), 0);
	grapes_config_value_int_default(tags, "source_multiplicity", &(ps->source_multiplicity), 3);


        tmp_str = grapes_config_value_str_default(tags, "bs_addr", NULL);
        ps->bs_addr = tmp_str ? strdup(tmp_str) : NULL;
        grapes_config_value_int_default(tags, "bs_port", &(ps->bs_port), 6000);
        
	tmp_str = grapes_config_value_str_default(tags, "filename", NULL);
	strcpy((ps->inc).filename, tmp_str ? tmp_str : "");
	tmp_str = grapes_config_value_str_default(tags, "AF", NULL);
	ps->l3 = tmp_str && (strcmp(tmp_str, "INET6") == 0) ? IP6 : IP4;

	free(tags);
	return 0;
}

int node_init(struct psinstance * ps, const char * config)
{
	char * my_addr;

	if (ps->iface)
		my_addr = iface_addr(ps->iface, ps->l3);
	else
		my_addr = default_ip_addr(ps->l3);
	if (my_addr == NULL)
	{
		fprintf(stderr, "[ERROR] cannot get a valid ip address\n");
		return -1;
	}
	ps->my_sock = net_helper_init(my_addr, ps->port, config);
	free(my_addr);

	if (ps->my_sock)
		return 0;
	else
		return -2;
}

struct psinstance * psinstance_create(const char * config)
{
	struct psinstance * ps = NULL;
	struct nodeID * srv;
	int res;

	ps = malloc(sizeof(struct psinstance));
        memset(ps, 0, sizeof(struct psinstance));
        
        ps->chunk_time_interval = 0;
        ps->chunk_offer_interval = 1000000/25;  // microseconds divided by frame (chunks) per second
        config_parse(ps, config);
        res = node_init(ps, config);
        if (ps->port >= 0 && ps->port < 65536 && res == 0)
        {
                ps->trader = chunk_trader_create(ps, config);
       
                ps->measure = measures_create(nodeid_static_str(ps->my_sock));
                ps->topology = topology_create(ps, config);
                ps->trader = chunk_trader_create(ps, config);
                streaming_timers_init(&(ps->timers), ps->chunk_offer_interval);
                ps->chunk_out = output_create(ps->measure, config);
                
                if(ps->bs_addr)
                {
                         //Normal peer (not the first)
                         fprintf(stderr,"Trying to contact the bootstrap node.\n");
                         srv = create_node(ps->bs_addr, ps->bs_port);
                       	 if (srv)
			 {	//the connection to the bootstrap node is successfull
				topology_node_insert(ps->topology, srv);
				nodeid_free(srv);
			 }else	//error to get the address of the peer
		         {
                                fprintf(stderr, "Error while connecting to bootstrap mode, exiting.\n");
                                psinstance_destroy(&ps);
                         }
                }
                else
                {
                        // creating the first peer (aka root peer, bootstrap node)
  		        fprintf(stderr,"Running as bootstrap node.\n");
                }

                
                ps->input = input_open((ps->inc).filename, 
                                        (ps->inc).fds, (ps->inc).fds_size, config);
        }
        else
        {
                psinstance_destroy(&ps);
        }

	return ps;
}

void psinstance_destroy(struct psinstance ** ps)
{
	if (ps && *ps)
	{
		if ((*ps)->measure)
			measures_destroy(&(*ps)->measure);
		if ((*ps)->topology)
			topology_destroy(&(*ps)->topology);
		if ((*ps)->chunk_out)
			output_destroy(&(*ps)->chunk_out);
		if ((*ps)->trader)
			chunk_trader_destroy(&(*ps)->trader);
		if ((*ps)->iface)
			free((*ps)->iface);
		if ((*ps)->my_sock)
			net_helper_deinit((*ps)->my_sock);
		if ((*ps)->input)
			input_close((*ps)->input);
                if ((*ps)->bs_addr)
                        free((*ps)->bs_addr);
                free(*ps);
		*ps = NULL;
	}
}

struct nodeID * psinstance_nodeid(const struct psinstance * ps)
{
	return ps->my_sock;
}


struct topology * psinstance_topology(const struct psinstance * ps)
{
	return ps->topology;
}

struct measures * psinstance_measures(const struct psinstance * ps)
{
	return ps->measure;
}

struct chunk_output * psinstance_output(const struct psinstance * ps)
{
	return ps->chunk_out;
}

const struct chunk_trader * psinstance_trader(const struct psinstance * ps)
{
	return ps->trader;
}

int8_t psinstance_send_offer(struct psinstance * ps)
{
	chunk_trader_send_offer(ps->trader);
	return 0;
}

int8_t psinstance_inject_chunk(struct psinstance * ps)
{
	struct chunk * new_chunk;
	int8_t res = 0;

	if (ps)
	{
		new_chunk = input_chunk(ps->input, &(ps->chunk_time_interval));
		if(new_chunk) 
		{
			if(!chunk_trader_add_chunk(ps->trader, new_chunk))
			{
				chunk_trader_push_chunk(ps->trader, new_chunk, ps->source_multiplicity);
				free(new_chunk);
			}
			else
				chunk_destroy(&new_chunk);
		}
		else
			res = -1;
	} else
		res = 1;
	return res;
}

int8_t psinstance_handle_msg(struct psinstance * ps)
	/* WARNING: this is a blocking function on the network socket */
{
	uint8_t buff[MSG_BUFFSIZE];
	struct nodeID *remote = NULL;
	struct chunk * c;
	int len;
	int8_t res = 0;

	len = recv_from_peer(ps->my_sock, &remote, buff, MSG_BUFFSIZE);
	if (len < 0) {
		fprintf(stderr,"[ERROR] Error receiving message. Maybe larger than %d bytes\n", MSG_BUFFSIZE);
		res = -1;
	}
	if (len > 0)
		switch (buff[0] /* Message Type */) {
			case MSG_TYPE_TMAN:
			case MSG_TYPE_NEIGHBOURHOOD:
			case MSG_TYPE_TOPOLOGY:
				dtprintf("Topo message received:\n");
				topology_message_parse(ps->topology, remote, buff, len);
				res = 1;
				break;
			case MSG_TYPE_CHUNK:
				dtprintf("Chunk message received:\n");
				c = chunk_trader_parse_chunk(ps->trader, remote,
                                                             buff, len);
                                if (c)
                                {
                                        if (!chunk_trader_add_chunk(ps->trader, c))
                                        {
                                                reg_chunk_receive(ps->measure, c);
                                                output_deliver(ps->chunk_out, c);
                                                free(c);
                                        } else
                                                chunk_destroy(&c);
                                }
				res = 2;
				break;
			case MSG_TYPE_SIGNALLING:
				dtprintf("Sign message received:\n");
				chunk_trader_msg_parse(ps->trader, remote, buff, len);
				res = 3;
				break;
			default:
				fprintf(stderr, "Unknown Message Type %x\n", buff[0]);
				res = -2;
		}

	if (remote)
		nodeid_free(remote);
	return res;
}

int psinstance_poll(struct psinstance *ps, suseconds_t delta)
{
	enum streaming_action required_action;
	int data_state;

	if (ps)
	{
		streaming_timers_set_timeout(&ps->timers, delta, ps->inc.fds[0] == -1);
		dtprintf("[DEBUG] timer: %lu %lu\n", ps->timers.sleep_timer.tv_sec, ps->timers.sleep_timer.tv_usec); 
		data_state = wait4data(ps->my_sock, &(ps->timers.sleep_timer), ps->inc.fds);

		required_action = streaming_timers_state_handler(&ps->timers, data_state);
		
                switch (required_action) {
			case OFFER_ACTION:
				dtprintf("Offer time!\n");
				psinstance_send_offer(ps);
				dtprintf("interval: %lu\n", ps->chunk_offer_interval);
				ps->chunk_offer_interval = chunk_trader_offer_interval(ps->trader);
				streaming_timers_update_offer_time(&ps->timers, ps->chunk_offer_interval);
				break;
			case INJECT_ACTION:
				dtprintf("Chunk seeding time!\n");
				psinstance_inject_chunk(ps);
				streaming_timers_update_chunk_time(&ps->timers, ps->chunk_time_interval);
				break;
			case PARSE_MSG_ACTION:
				dtprintf("Got a message from the world!!\n");
				psinstance_handle_msg(ps);
				break;
			case NO_ACTION:
				dtprintf("Nothing happens...\n");
			default:
				break;
		}
		if (streaming_timers_update_flag(&ps->timers))
				topology_update(ps->topology);
	}
	return data_state;
}

int8_t psinstance_topology_update(const struct psinstance * ps)
{
	if (ps && ps->topology)
		topology_update(ps->topology);
	return 0;
}

suseconds_t psinstance_offer_interval(const struct psinstance * ps)
{
	return ps->chunk_offer_interval;
}

int pstreamer_register_fds(const struct psinstance * ps, fd_register_f func, void *handler)
{
	register_network_fds(ps->my_sock, func, handler);
	return 0;
}

suseconds_t psinstance_network_periodic(struct psinstance * ps)
{
	struct timeval interval;
	suseconds_t delay = 500;

	if (ps && ps->my_sock)
	{
		net_helper_periodic(ps->my_sock, &interval);
		delay = interval.tv_sec * 1000 + interval.tv_usec /1000;
	}
	return delay;
}

int psinstance_ip_address(const struct psinstance *ps, char * ip, int len)
{
	int res = -1;
	if (ps && ps->my_sock)
		res = node_ip(ps->my_sock, ip, len);
	return res;
}

int psinstance_port(const struct psinstance *ps)
{
	int res = -1;
	if (ps && ps->my_sock)
		return node_port(ps->my_sock);
	return res;
}
