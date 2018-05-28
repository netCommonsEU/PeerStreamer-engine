#ifndef __CHUNK_ATTRIBUTES_H__
#define __CHUNK_ATTRIBUTES_H__

#include<stdint.h>

#include<chunk.h>


struct chunk_attributes;

int8_t chunk_attributes_init(struct chunk *c);

int8_t chunk_attributes_deinit(struct chunk * c);

uint16_t chunk_attributes_get_hopcount(const struct chunk * c);

int8_t chunk_attributes_update_upon_sending(struct chunk *c);

int8_t chunk_attributes_update_upon_reception(struct chunk *c);

#endif
