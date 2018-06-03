#include<malloc.h>
#include<assert.h>
#include<output.h>
#include<chunk.h>
#include<string.h>


struct chunk * create_chunk(int flowid, int id)
{
	struct chunk * c;
	char msg[80];

	sprintf(msg, "ciao - %d", id);

	c = malloc(sizeof(struct chunk));
	c->id = id;
        c->flow_id = flowid;
	c->timestamp = 2;
	c->attributes = NULL;
	c->attributes_size = 0;
	c->data = (uint8_t *) strdup(msg);
	c->size = strlen((char *) c->data);
	return c;
}

void destroy_chunk(struct chunk **c)
{
	if (c && *c)
	{
		free((*c)->data);
		free(*c);
		*c = NULL;
	}
}

void output_create_test()
{
	struct chunk_output * co;
	struct measures * m;

	m = measures_create("dump");

	co = output_create(NULL, NULL);
	assert(co);
	output_destroy(&co);
	assert(co == NULL);

	co = output_create(NULL, "dechunkiser=dummy");
	assert(co);
	output_destroy(&co);
	assert(co == NULL);

	co = output_create(m, "dechunkiser=dummy");
	assert(co);
	output_destroy(&co);
	assert(co == NULL);

	measures_destroy(&m);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_deliver_test()
{
	struct chunk * c;
	struct chunk_output * outg;

	c = create_chunk(1,1);
	outg = output_create(NULL, "dechunkiser=dummy");

	output_deliver(NULL, NULL);
	output_deliver(outg, NULL);
	output_deliver(outg, c);
	destroy_chunk(&c);

	c = create_chunk(1,2);
	output_deliver(outg, c);
	destroy_chunk(&c);

	c = create_chunk(1,5);
	output_deliver(outg, c);
	destroy_chunk(&c);

	c = create_chunk(1,3);
	output_deliver(outg, c);
	destroy_chunk(&c);

	c = create_chunk(1,4);
	output_deliver(outg, c);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_reordering_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "dechunkiser=dummy,outbuff_size=3");

	c = create_chunk(1,4);
	res = output_deliver(outg, c);
        printf("%d\n",res);
        assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1,3);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1,6);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1,5);
	res = output_deliver(outg, c);
	assert(res == 2);
	destroy_chunk(&c);

	c = create_chunk(1,15);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1,18);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_reordering2_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "dechunkiser=dummy,outbuff_size=4");

	c = create_chunk(1,4);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 6);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1, 8);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1, 10);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 9);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1, 7);
	res = output_deliver(outg, c);
	assert(res == 4);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_noreordering_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "outbuff_reorder=0,dechunkiser=dummy,outbuff_size=3");

	c = create_chunk(1, 4);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 3);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 6);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 5);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 15);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 18);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_reordering_flawless_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "dechunkiser=dummy,outbuff_size=3");

	c = create_chunk(1, 1);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 2);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 3);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1, 4);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1,5);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1,6);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_ordering_duplicates_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "dechunkiser=dummy,outbuff_size=3");

	c = create_chunk(1,4);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1,6);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1,6);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void output_destroy_test()
{
	struct chunk * c;
	struct chunk_output * outg;
	int res;

	outg = output_create(NULL, "dechunkiser=dummy,outbuff_size=10");

	c = create_chunk(1,5);
	res = output_deliver(outg, c);
	assert(res == 1);
	destroy_chunk(&c);

	c = create_chunk(1,9);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	c = create_chunk(1,8);
	res = output_deliver(outg, c);
	assert(res == 0);
	destroy_chunk(&c);

	output_destroy(&outg);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	output_create_test();
	output_deliver_test();
	output_reordering_test();
	output_reordering2_test();
	output_reordering_flawless_test();
	output_ordering_duplicates_test();
	output_noreordering_test();
	output_destroy_test();
	return 0;
}

