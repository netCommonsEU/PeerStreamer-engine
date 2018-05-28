#include<stdint.h>
#include<chunk_attributes.h>
#include<stdlib.h>

struct chunk_attributes {
	uint16_t hopcount;
} __attribute__((packed));

int8_t chunk_attributes_init(struct chunk *c)
{
	int8_t res = -1;
	struct chunk_attributes * attr;

	if (c)
	{
		c->attributes_size = sizeof(struct chunk_attributes);
		c->attributes = attr = malloc(c->attributes_size);
		attr->hopcount = 0;
	}
	return res;
}

uint16_t chunk_attributes_get_hopcount(const struct chunk * c)
{
	struct chunk_attributes * attr;

	attr = c->attributes;
	return attr->hopcount;
}

int8_t chunk_attributes_deinit(struct chunk * c)
{
	if (c && c->attributes)
	{
		free(c->attributes);
		c->attributes_size = 0;
		return 0;
	}
	return -1;
}

int8_t chunk_attributes_update_upon_reception(struct chunk *c)
{
	struct chunk_attributes * attr;

	if (c && c->attributes)
	{
		attr = c->attributes;
		attr->hopcount++;
	}
	return -1;
}

int8_t chunk_attributes_update_upon_sending(struct chunk *c)
{
	return 0;
}

