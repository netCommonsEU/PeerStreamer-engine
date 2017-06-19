#include<peer_metadata.h>
#include<malloc.h>
#include<string.h>

int8_t metadata_update(struct metadata *m, uint16_t cb_size, uint8_t neigh_size)
{
	if (m)
	{
		m->cb_size = cb_size;
		m->neigh_size = neigh_size;
		return 0;
	}
	return -1;
}

int8_t peer_set_metadata(struct  peer *p, const struct metadata *m)
{
	if (p && m)
	{
		if (!(p->metadata))
			p->metadata = malloc(sizeof(struct metadata));
		memmove(p->metadata, m, sizeof(struct metadata));
		return 0;
	}
	return -1;
}

uint16_t peer_cb_size(const struct peer *p)
{
	if (p && p->metadata)
		return ((struct metadata *)p->metadata)->cb_size;
	return DEFAULT_PEER_CBSIZE;
}
