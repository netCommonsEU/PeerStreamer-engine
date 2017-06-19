#ifndef __PEER_METADATA_H__
#define __PEER_METADATA_H__

#include<stdint.h>
#include<peer.h>

#define DEFAULT_PEER_CBSIZE 50

struct metadata {
  uint16_t cb_size;
  uint8_t neigh_size;
} __attribute__((packed));


int8_t metadata_update(struct metadata *m, uint16_t cb_size, uint8_t neigh_size);

int8_t peer_set_metadata(struct  peer *p, const struct metadata *m);

uint16_t peer_cb_size(const struct peer *p);

#endif
