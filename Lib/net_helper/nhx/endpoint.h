#ifndef __ENDPOING_H__
#define __ENDPOING_H__

#include<net_helper.h>
#include<list.h>
#include<stdint.h>
#include<stdlib.h>


struct endpoint;

struct endpoint * endpoint_create(const struct nodeID * node, size_t frag_size, uint16_t max_pkt_age);

void endpoint_destroy(struct endpoint ** e);

int8_t endpoint_cmp(const void * e1, const void *e2);

struct list_head * endpoint_enqueue_outgoing_packet(struct endpoint * e, const struct nodeID * src, const uint8_t * data, size_t data_len);

#endif
