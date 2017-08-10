#include<fragmented_packet.h>
#include<sys/time.h>

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif 

time_t fragmented_packet_creation_timestamp(const struct fragmented_packet *fp)
{
	if (fp)
		return fp->creation_timestamp;
	return 0;
}

packet_id_t fragmented_packet_id(const struct fragmented_packet *fp)
{
	if (fp)
		return fp->packet_id;
	return 0;
}

struct fragmented_packet * fragmented_packet_create(packet_id_t id, const struct nodeID * from, const struct nodeID *to, const uint8_t * data, size_t data_size, size_t frag_size, struct list_head ** msgs)
{
	struct fragmented_packet * fp = NULL;
	const uint8_t * data_ptr = data;
	struct list_head * list = NULL;
	frag_id_t i;

	if (data && data_size > 0 && frag_size > 0)
	{
		fp = malloc(sizeof(struct fragmented_packet));
		fp->packet_id = id;
		fp->creation_timestamp = time(NULL);
		INIT_LIST_HEAD(&(fp->list));
		if (!frag_size)
			frag_size = data_size;
		fp->frag_num = data_size/frag_size;
		if (data_size % frag_size)
			fp->frag_num++;
		fp->frags = malloc(sizeof(struct fragment) * fp->frag_num);
		for (i = 0; i < fp->frag_num; i++)
		{
			fragment_init(&(fp->frags[i]), from, to, i, data+(i*frag_size), MIN(frag_size, data_size), list);
			data_ptr += frag_size; 
			data_size -= frag_size;
			if (i == 0)
				list = fragment_list_element(&(fp->frags[i]));
		}
		*msgs = list;
	}
	return fp;
}

void fragmented_packet_destroy(struct fragmented_packet ** fp)
{
	frag_id_t i;

	if (fp && *fp)
	{
		for (i = 0; i < (*fp)->frag_num; i++)
			fragment_deinit(&(*fp)->frags[i]);
		free((*fp)->frags);
		free(*fp);
		*fp = NULL;
	}
}
