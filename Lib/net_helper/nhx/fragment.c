#include<fragment.h>
#include<string.h>

int8_t fragment_init(struct fragment * f, const struct nodeID * from, const struct nodeID * to, frag_id_t id, const uint8_t * data, size_t data_size, struct list_head * list)
{
	int8_t res = -1;

	if (f && from && to && data && data_size > 0)
	{
		res = net_msg_init((struct net_msg *) f, NET_FRAGMENT, from, to, list);
		if (res == 0)
		{
			f->data_size = data_size;
			f->data = malloc(sizeof(uint8_t) * data_size);
			memmove(f->data, data, f->data_size);
			f->id = id;
		}
	}

	return res;
}

void fragment_deinit(struct fragment * f)
{
	if (f)
	{
		if(f->data)
			free(f->data);
		net_msg_deinit((struct net_msg *) f);
	}
}

struct list_head * fragment_list_element(struct fragment *f)
{
	if (f)
		return &((struct net_msg*)f)->list;
	return NULL;
}
