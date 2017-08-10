#ifndef __PACKET_BUCKET_H__
#define __PACKET_BUCKET_H__

#include<stdlib.h>
#include<fragment.h>
#include<stdint.h>


struct packet_bucket;

struct packet_bucket * packet_bucket_create(size_t frag_size, uint16_t max_pkt_age);

void packet_bucket_destroy(struct packet_bucket ** pb);

struct list_head * packet_bucket_add_packet(struct packet_bucket * pb, const struct nodeID * src, const struct nodeID *dst, packet_id_t pid, const uint8_t *data, size_t data_len);

#endif
