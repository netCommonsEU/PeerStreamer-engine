#ifndef __NETWORK_MANAGER_H__
#define __NETWORK_MANAGER_H__

#include<stdint.h>
#include<stdlib.h>
#include<net_helper.h>
#include<fragmented_packet.h>
#include<fragment.h>

struct network_manager;

struct network_manager * network_manager_create(const char * config);

void network_manager_destroy(struct network_manager ** nm);

/***************************Ougoing*********************************/
int8_t network_manager_enqueue_outgoing_packet(struct network_manager *nm, const struct nodeID *src, const struct nodeID * dst, const uint8_t * data, size_t data_len);

struct net_msg * network_manager_pop_outgoing_net_msg(struct network_manager *nm);

/************************Incoming*************************************/

int8_t network_manager_add_incoming_fragment(struct network_manager * nm, const struct fragment * f);

uint8_t * network_manager_pop_incoming_packet(struct network_manager *nm, const struct nodeID * src, packet_id_t id);

/*************************PolicyDriven***********************************/

int8_t network_manager_enqueue_outgoing_fragment(struct network_manager *nm, const struct nodeID * dst, packet_id_t id, frag_id_t fid);

#endif
