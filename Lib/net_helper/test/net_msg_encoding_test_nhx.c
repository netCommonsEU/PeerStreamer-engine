#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<net_msg.h>
#include<fragment.h>
#include<frag_request.h>
#include<net_helper.h>


void fragment_encode_test()
{
	struct fragment frag, *neo;
	struct nodeID * src, *dst;
	uint8_t buff[100];
	int8_t res;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.1", 6020);

	fragment_init(&frag, src, dst, 42, 7, 3, (uint8_t*) "ciao", 5, NULL);
	res = fragment_encode(&frag, buff, 100);
	assert(res == 0);
	
	neo = fragment_decode(dst, src, buff, 100);
	assert(neo);

	assert(neo->pid == frag.pid);
	assert(neo->id == frag.id);
	assert(neo->frag_num == frag.frag_num);
	assert(neo->data_size == frag.data_size);
	assert(neo->data);
	assert(strcmp((char*)neo->data, "ciao") == 0);

	nodeid_free(src);
	nodeid_free(dst);
	fragment_deinit(&frag);
	fragment_deinit(neo);
	free(neo);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void frag_request_encode_test()
{
	struct frag_request * fr, *neo;
	struct nodeID * src, *dst;
	uint8_t buff[100];
	int8_t res;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.1", 6020);

	fr = frag_request_create(src, dst, 42, 7, NULL);
	res = frag_request_encode(fr, buff, 100);
	assert(res == 0);
	
	neo = frag_request_decode(dst, src, buff, 100);
	assert(neo);

	assert(neo->pid == fr->pid);
	assert(neo->id == fr->id);

	nodeid_free(src);
	nodeid_free(dst);
	frag_request_destroy(&fr);
	frag_request_destroy(&neo);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	fragment_encode_test();
	frag_request_encode_test();
	return 0;
}
