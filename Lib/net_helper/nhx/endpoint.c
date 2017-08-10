#include<endpoint.h>
#include<net_helper.h>
#include<packet_bucket.h>
#include<fragment.h>

struct endpoint {  // do not move node parameter
	struct nodeID * node;
	struct packet_bucket * incoming;
	struct packet_bucket * outgoing;
	packet_id_t out_id;
};

struct list_head * endpoint_enqueue_outgoing_packet(struct endpoint * e, const struct nodeID * src, const uint8_t * data, size_t data_len)
{
	struct list_head * res = NULL;
	if (e && src && data && data_len > 0)
		res = packet_bucket_add_packet(e->outgoing, src, e->node, e->out_id++, data, data_len);
	return res;
}

struct endpoint * endpoint_create(const struct nodeID * node, size_t frag_size, uint16_t max_pkt_age)
{
	struct endpoint * e = NULL;
	if (node)
	{
		e = malloc(sizeof(struct endpoint));
		e->node = nodeid_dup(node);
		e->incoming = packet_bucket_create(frag_size, max_pkt_age);
		e->outgoing = packet_bucket_create(frag_size, max_pkt_age);
		e->out_id = 0;
	}
	return e;
}

void endpoint_destroy(struct endpoint ** e)
{
	if (e && *e)
	{
		packet_bucket_destroy(&(*e)->incoming);
		packet_bucket_destroy(&(*e)->outgoing);
		nodeid_free((*e)->node);
		free(*e);
		*e = NULL;
	}
}

int8_t endpoint_cmp(const void * e1, const void *e2)
{
	if (e1 && e2)
		if (nodeid_equal(((struct endpoint *)e1)->node, ((struct endpoint *)e2)->node))
			return 0;
		return nodeid_cmp(((struct endpoint *)e1)->node, ((struct endpoint *)e2)->node) > 0 ? 1 : -1;
	return 0;
}
