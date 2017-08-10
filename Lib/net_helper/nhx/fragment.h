#ifndef __FRAGMENT_H__
#define __FRAGMENT_H__

#include<net_msg.h>

typedef uint16_t frag_id_t;
typedef uint16_t packet_id_t;

struct fragment {  // extends net_msg, do not move nm parameter
	struct net_msg nm;
	frag_id_t id;
	packet_id_t pid;
	size_t data_size;
	uint8_t * data;
};

int8_t fragment_init(struct fragment * f, const struct nodeID * from, const struct nodeID * to, frag_id_t id, const uint8_t * data, size_t data_size, struct list_head * list);

void fragment_deinit(struct fragment * f);

struct list_head * fragment_list_element(struct fragment *f);

#endif
