#include<packet_bucket.h>
#include<fragmented_packet.h>
#include<malloc.h>
#include<ord_set.h>
#include<list.h>
#include<stdlib.h>


struct packet_bucket {
	struct list_head * packet_list;
	struct ord_set * packet_set;
	size_t frag_size; 
	uint16_t max_pkt_age;
};

void packet_bucket_periodic_refresh(struct packet_bucket * pb)
{
	struct list_head * pos, *tmp;
	uint8_t time_check = 1;
	time_t current_time;
	struct fragmented_packet * fp;

	current_time = time(NULL);
	pos = pb->packet_list;
	while (pos && pos != pb->packet_list && time_check)
	{
		tmp = pos->next;
		fp = list_entry(pos, struct fragmented_packet, list);
		if (current_time - fragmented_packet_creation_timestamp(fp) >= pb->max_pkt_age)
		{
			list_del(pos);
			ord_set_remove(pb->packet_set, fp, 0);
			fragmented_packet_destroy(&fp);
		} else
			time_check = 0;
		pos = tmp;
	}
}

struct list_head * packet_bucket_add_packet(struct packet_bucket * pb, const struct nodeID * src, const struct nodeID *dst, packet_id_t pid, const uint8_t *data, size_t data_len)
{
	struct fragmented_packet * fp;
	void * insert_res;
	struct list_head * res = NULL;

	if (pb && src && dst && data && data_len > 0)
	{
		packet_bucket_periodic_refresh(pb);
		fp = fragmented_packet_create(pid, src, dst, data, data_len, pb->frag_size, &res);
		insert_res = ord_set_insert(pb->packet_set, fp, 0);
		if (fp == insert_res)
		{
			if (pb->packet_list)
				list_add(&(fp->list), pb->packet_list);
			else {
				pb->packet_list = &(fp->list);
				INIT_LIST_HEAD(pb->packet_list);
			}
		} else 
			fragmented_packet_destroy(&fp);
	}

	return res;
}

int8_t packet_cmp(const void * p1, const void * p2)
{
	const struct fragmented_packet * fp1 = p1;
	const struct fragmented_packet * fp2 = p2;
	packet_id_t i1, i2;
	i1 = fragmented_packet_id(fp1);
	i2 = fragmented_packet_id(fp2);
	if (i1 == i2)
		return 0;
	return i1 > i2 ? 1 : -1;	
}

struct packet_bucket * packet_bucket_create(size_t frag_size, uint16_t max_pkt_age)
{
	struct packet_bucket * pb = NULL;

	pb = malloc(sizeof(struct packet_bucket));
	pb->packet_set = ord_set_new(10, packet_cmp);
	pb->packet_list = NULL;
	pb->frag_size = frag_size;
	pb->max_pkt_age = max_pkt_age;
	return pb;
}

void packet_bucket_destroy(struct packet_bucket ** pb)
{
	void * fp, * tmp;
	if (pb && *pb)
	{
		ord_set_for_each_safe(fp, (*pb)->packet_set, tmp)
		{
			ord_set_remove((*pb)->packet_set, fp, 0);
			fragmented_packet_destroy((struct fragmented_packet **) &fp);
		}
		ord_set_destroy(&((*pb)->packet_set), 0);
		free(*pb);
		*pb = NULL;
	}
}


