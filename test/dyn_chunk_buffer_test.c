#include<assert.h>
#include<dyn_chunk_buffer.h>
#include<chunk.h>
#include<unistd.h>
#include<scheduler_common.h>


void dyn_chunk_buffer_create_test()
{
	struct dyn_chunk_buffer * dcb;

	dcb = dyn_chunk_buffer_create();
	assert(dcb);
	dyn_chunk_buffer_destroy(&dcb);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void dyn_chunk_buffer_add_chunk_test()
{
	struct dyn_chunk_buffer * dcb=NULL;
	struct chunk c = {0};
	int8_t res;

	res = dyn_chunk_buffer_add_chunk(dcb, NULL);
	assert(res);

	dcb = dyn_chunk_buffer_create();
	res = dyn_chunk_buffer_add_chunk(dcb, NULL);
	assert(res);

	res = dyn_chunk_buffer_add_chunk(dcb, &c);
	assert(res==0);

	c.id = 3;
	c.timestamp = 3;
	res = dyn_chunk_buffer_add_chunk(dcb, &c);
	assert(res==0);

	dyn_chunk_buffer_destroy(&dcb);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void dyn_chunk_buffer_get_chunk_test()
{
	struct dyn_chunk_buffer * dcb=NULL;
	struct chunk c = {0};
   	const struct chunk *ptr;

	ptr = dyn_chunk_buffer_get_chunk(NULL, 0, 0);
	assert(ptr == NULL);

	dcb = dyn_chunk_buffer_create();
	ptr = dyn_chunk_buffer_get_chunk(dcb, 0, 0);
	assert(ptr == NULL);

	dyn_chunk_buffer_add_chunk(dcb, &c);
	ptr = dyn_chunk_buffer_get_chunk(dcb, 0, 1);
	assert(ptr == NULL);
	ptr = dyn_chunk_buffer_get_chunk(dcb, 1, 0);
	assert(ptr == NULL);

	ptr = dyn_chunk_buffer_get_chunk(dcb, 0, 0);
	assert(ptr);

	c.id = 3;
	c.timestamp = 3;
	dyn_chunk_buffer_add_chunk(dcb, &c);
	ptr = dyn_chunk_buffer_get_chunk(dcb, 0, 0);
	assert(ptr);

	dyn_chunk_buffer_destroy(&dcb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void dyn_chunk_buffer_to_multiset_test()
{
	struct dyn_chunk_buffer * dcb=NULL;
	struct chunk c = {0};
	struct chunkID_multiSet * set;

	set = dyn_chunk_buffer_to_multiset(dcb);
	assert(set == NULL);

	dcb = dyn_chunk_buffer_create();
	set = dyn_chunk_buffer_to_multiset(dcb);
	assert(set);
	assert(chunkID_multiSet_total_size(set) == 0);
	chunkID_multiSet_free(set);

	dyn_chunk_buffer_add_chunk(dcb, &c);
	set = dyn_chunk_buffer_to_multiset(dcb);
	assert(set);
	assert(chunkID_multiSet_total_size(set) == 1);
	chunkID_multiSet_free(set);

	c.id = 3;
	c.timestamp = 3;
	dyn_chunk_buffer_add_chunk(dcb, &c);
	set = dyn_chunk_buffer_to_multiset(dcb);
	assert(set);
	assert(chunkID_multiSet_total_size(set) == 2);
	chunkID_multiSet_free(set);

	c.id = 4;
	c.timestamp = 4;
	dyn_chunk_buffer_add_chunk(dcb, &c);
	set = dyn_chunk_buffer_to_multiset(dcb);
	assert(set);
	assert(chunkID_multiSet_total_size(set) == 3);
	chunkID_multiSet_free(set);

	dyn_chunk_buffer_destroy(&dcb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void dyn_chunk_buffer_reshaping_test()
{
	struct dyn_chunk_buffer * dcb=NULL;
	struct chunk c = {0};
	int i;

	dcb = dyn_chunk_buffer_create();
	for (i=0; i< 100; i++)
	{
		c.id = i;
		c.timestamp = i;
		dyn_chunk_buffer_add_chunk(dcb, &c);
	}

	dyn_chunk_buffer_destroy(&dcb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void dyn_chunk_buffer_to_idarray_test()
{
	struct dyn_chunk_buffer * dcb=NULL;
	struct chunk c = {0};
	struct sched_chunkID * ids;
	uint32_t len,i;
	int sum;

	ids = dyn_chunk_buffer_to_idarray(dcb, &len);
	assert(ids==NULL);
	assert(len==0);

	dcb = dyn_chunk_buffer_create();
	ids = dyn_chunk_buffer_to_idarray(dcb, &len);
	assert(ids==NULL);
	assert(len==0);

	dyn_chunk_buffer_add_chunk(dcb, &c);
	ids = dyn_chunk_buffer_to_idarray(dcb, &len);
	assert(ids);
	assert(len==1);
	assert(ids[0].chunk_id == 0);
	assert(ids[0].flow_id == 0);
	free(ids);
	dyn_chunk_buffer_destroy(&dcb);

	dcb = dyn_chunk_buffer_create();
	sum = 0;
	for (i=0; i< 10; i++)
	{
		c.id = i;
		c.timestamp = i;
		dyn_chunk_buffer_add_chunk(dcb, &c);
		sum += i;
	}
	ids = dyn_chunk_buffer_to_idarray(dcb, &len);
	assert(ids);
	assert(len==10);
	for (i=0; i< 10; i++)
		sum -= ids[i].chunk_id;
	assert(sum == 0);
	free(ids);

	dyn_chunk_buffer_destroy(&dcb);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	dyn_chunk_buffer_create_test();
	dyn_chunk_buffer_add_chunk_test();
	dyn_chunk_buffer_get_chunk_test();
	dyn_chunk_buffer_to_multiset_test();
	dyn_chunk_buffer_reshaping_test();
	dyn_chunk_buffer_to_idarray_test();
	return 0;
}
