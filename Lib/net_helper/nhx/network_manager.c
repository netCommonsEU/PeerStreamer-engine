#include<network_manager.h>
#include<malloc.h>
#include<endpoint.h>
#include<ord_set.h>
#include<grapes_config.h>

#define DEFAULT_FRAG_SIZE 1200
#define DEFAULT_PKT_MAX_AGE 4


struct network_manager {
	struct list_head * outqueue;
	struct ord_set * endpoints;
	size_t frag_size;
	uint16_t max_pkt_age; // in seconds
};

struct network_manager * network_manager_create(const char * config)
{
	struct network_manager * nm ;
	struct tag * tags = NULL;
	int frag_size = DEFAULT_FRAG_SIZE;
	int max_pkt_age = DEFAULT_PKT_MAX_AGE;


	nm = malloc(sizeof(struct network_manager));
	nm->endpoints = ord_set_new(10, endpoint_cmp);
	nm->outqueue = NULL;

	if (config)
	{
		tags = grapes_config_parse(config);
		grapes_config_value_int_default(tags, "frag_size", &frag_size, DEFAULT_FRAG_SIZE);
		grapes_config_value_int_default(tags, "max_pkt_age", &max_pkt_age, DEFAULT_PKT_MAX_AGE);
		free(tags);
	}
	nm->frag_size = frag_size;
	nm->max_pkt_age = max_pkt_age;
	return nm;
}

void network_manager_destroy(struct network_manager ** nm)
{
	void * e, * tmp;

	if (nm && *nm)
	{
		ord_set_for_each_safe(e, (*nm)->endpoints, tmp)
		{
			ord_set_remove((*nm)->endpoints, (void *) e, 0);
			endpoint_destroy((struct endpoint **)&e);	
		}
		ord_set_destroy(&((*nm)->endpoints), 0);
		free(*nm);
		*nm = NULL;
	}
}

int8_t network_manager_enqueue_outgoing_packet(struct network_manager *nm, const struct nodeID *src, const struct nodeID * dst, const uint8_t * data, size_t data_len)
{
	int8_t res = -1;
	struct endpoint * e;
	struct list_head * frag_list;

	if (nm && dst && data && data_len > 0)
	{
		e = ord_set_find(nm->endpoints, dst);
		if (!e)
		{
			e = endpoint_create(dst, nm->frag_size, nm->max_pkt_age);
			ord_set_insert(nm->endpoints, (void *)e, 0);
		}
		frag_list = endpoint_enqueue_outgoing_packet(e, src, data, data_len);
		if (frag_list)
		{
			if (nm->outqueue)
				list_splice(nm->outqueue, frag_list);
			else
				nm->outqueue = frag_list;
			res = 0;
		}
	}

	return res;
}


struct net_msg * network_manager_pop_outgoing_net_msg(struct network_manager *nm)
{
	struct net_msg * m = NULL;
	struct list_head * el;

	if (nm)
	{
		el = nm->outqueue;
		if (el)
		{
			list_del(el);
			m = list_entry(el, struct net_msg, list);
		}
	}
	return m;
}
