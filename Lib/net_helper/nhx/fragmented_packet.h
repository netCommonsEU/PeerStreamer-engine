#ifndef __FRAGMENTED_PACKET_H__
#define __FRAGMENTED_PACKET_H__

#include<time.h>
#include<fragment.h>
#include<net_helper.h>


struct fragmented_packet {
	time_t creation_timestamp;
	struct fragment * frags;
	frag_id_t frag_num;
	struct list_head list;
	packet_id_t packet_id;
};

void fragmented_packet_destroy(struct fragmented_packet **);

packet_id_t fragmented_packet_id(const struct fragmented_packet *fp);

time_t fragmented_packet_creation_timestamp(const struct fragmented_packet *fp);

struct fragmented_packet * fragmented_packet_create(packet_id_t id, const struct nodeID * from, const struct nodeID *to, const uint8_t * data, size_t data_size, size_t frag_size, struct list_head ** msgs);

#endif
