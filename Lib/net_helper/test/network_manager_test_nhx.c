#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<network_manager.h>
#include<frag_request.h>


void network_manager_create_test()
{
	struct network_manager * nm;

	nm = network_manager_create(NULL);
	assert(nm);

	network_manager_destroy(&nm);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_manager_enqueue_outgoing_packet_test()
{
	struct network_manager * nm;
	struct nodeID *src, *dst;
	uint8_t data[] = "ciao";
	size_t data_len = 5;
	int8_t res;

	nm = network_manager_create(NULL);
	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.2", 6000);

	res = network_manager_enqueue_outgoing_packet(NULL, NULL, NULL, NULL, 0);
	assert(res < 0);
	res = network_manager_enqueue_outgoing_packet(nm, NULL, NULL, NULL, 0);
	assert(res < 0);
	res = network_manager_enqueue_outgoing_packet(nm, src, dst, NULL, 0);
	assert(res < 0);
	res = network_manager_enqueue_outgoing_packet(nm, src, dst, data, 0);
	assert(res < 0);

	res = network_manager_enqueue_outgoing_packet(nm, src, dst, data, data_len);
	assert(res == 0);

	network_manager_destroy(&nm);
	nodeid_free(src);
	nodeid_free(dst);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_manager_pop_outgoing_net_msg_test()
{
	struct network_manager * nm = NULL;
	struct nodeID *src, *dst;
	uint8_t data[] = "ciao";
	size_t data_len = 5;
	struct net_msg * msg;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.2", 6000);

	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	nm = network_manager_create("frag_size=3");

	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	network_manager_enqueue_outgoing_packet(nm, src, dst, data, data_len);

	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg != NULL);
	assert(((struct fragment *)msg)->data_size == 3);
	assert(strncmp((char *)((struct fragment *)msg)->data, "cia", 3) == 0);

	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(((struct fragment *)msg)->data_size == 2);
	assert(strncmp((char *)((struct fragment *)msg)->data, "o", 2) == 0);
	assert(msg != NULL);

	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	network_manager_destroy(&nm);
	nodeid_free(src);
	nodeid_free(dst);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_manager_add_incoming_fragment_test()
{
	struct network_manager *nm = NULL;
	struct nodeID *src, *dst;
	struct fragment f;
	struct frag_request *fr;
	struct net_msg * msg;
	packet_state_t res;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.2", 6000);

	res = network_manager_add_incoming_fragment(nm, NULL);
	assert(res == PKT_ERROR);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	nm = network_manager_create(NULL);
	res = network_manager_add_incoming_fragment(nm, NULL);
	assert(res == PKT_ERROR);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	fragment_init(&f, src, dst, 0, 2, 0, (uint8_t *)"ci", 2, NULL);
	res = network_manager_add_incoming_fragment(nm, &f);
	assert(res == PKT_LOADING);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);
	fragment_deinit(&f);

	fragment_init(&f, src, dst, 0, 2, 1, (uint8_t *)"ao", 3, NULL);
	res = network_manager_add_incoming_fragment(nm, &f);
	assert(res == PKT_READY);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);
	fragment_deinit(&f);
	
	fragment_init(&f, src, dst, 1, 2, 1, (uint8_t *)"foo", 3, NULL);
	res = network_manager_add_incoming_fragment(nm, &f);
	assert(res == PKT_LOADING);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg != NULL);
	fr = (struct frag_request *) msg;
	assert(fr->pid == 1);
	assert(fr->id == 0);
	frag_request_destroy(&fr);
	fragment_deinit(&f);
	
	fragment_init(&f, src, dst, 1, 2, 0, (uint8_t *)"bar", 4, NULL);
	res = network_manager_add_incoming_fragment(nm, &f);
	assert(res == PKT_READY);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);
	fragment_deinit(&f);

	network_manager_destroy(&nm);
	nodeid_free(src);
	nodeid_free(dst);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_manager_pop_incoming_packet_test()
{
	struct network_manager *nm = NULL;
	struct nodeID *src, *dst;
	struct fragment f;
	int8_t res;
	uint8_t buff[200];
	size_t maxsize = 200, size;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.2", 6000);
	size = maxsize;

	res = network_manager_pop_incoming_packet(nm, NULL, 0, NULL, NULL);
	assert(res < 0);

	nm = network_manager_create(NULL);

	res = network_manager_pop_incoming_packet(nm, src, 0, buff, NULL);
	assert(res < 0);

	res = network_manager_pop_incoming_packet(nm, src, 0, NULL, &size);
	assert(res < 0);

	res = network_manager_pop_incoming_packet(nm, NULL, 0, buff, &size);
	assert(res < 0);

	res = network_manager_pop_incoming_packet(nm, src, 0, buff, &size);
	assert(res < 0);

	fragment_init(&f, src, dst, 0, 2, 0, (uint8_t *)"ci", 2, NULL);
	network_manager_add_incoming_fragment(nm, &f);
	fragment_deinit(&f);

	fragment_init(&f, src, dst, 0, 2, 1, (uint8_t *)"ao", 3, NULL);
	network_manager_add_incoming_fragment(nm, &f);
	fragment_deinit(&f);

	res = network_manager_pop_incoming_packet(nm, src, 0, buff, &size);
	assert(res == 0);
	assert(size == 5);
	assert(strcmp("ciao", (char *)buff) == 0);

	fragment_init(&f, src, dst, 1, 20, 0, (uint8_t *)"foo", 4, NULL);
	network_manager_add_incoming_fragment(nm, &f);
	fragment_deinit(&f);

	size = maxsize;
	res = network_manager_pop_incoming_packet(nm, src, 1, buff, &size);
	assert(res == 0);
	assert(size == 4);
	assert(strcmp("foo", (char *)buff) == 0);

	network_manager_destroy(&nm);
	nodeid_free(src);
	nodeid_free(dst);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void network_manager_enqueue_outgoing_fragment_test()
{
	struct network_manager *nm = NULL;
	struct nodeID *src, *dst;
	struct net_msg * msg;
	int8_t res;

	src = create_node("10.0.0.1", 6000);
	dst = create_node("10.0.0.2", 6000);

	res = network_manager_enqueue_outgoing_fragment(nm, dst, 0, 0);
	assert(res < 0);

	nm = network_manager_create("frag_size=7");

	res = network_manager_enqueue_outgoing_fragment(nm, NULL, 0, 0);
	assert(res < 0);

	network_manager_enqueue_outgoing_packet(nm, src, dst, (uint8_t*)"ciao", 5);
	msg = network_manager_pop_outgoing_net_msg(nm);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg == NULL);

	res = network_manager_enqueue_outgoing_fragment(nm, dst, 4, 0);
	assert(res < 0);

	res = network_manager_enqueue_outgoing_fragment(nm, dst, 0, 0);
	assert(res == 0);
	msg = network_manager_pop_outgoing_net_msg(nm);
	assert(msg != NULL);

	network_manager_destroy(&nm);
	nodeid_free(src);
	nodeid_free(dst);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	network_manager_create_test();
	network_manager_enqueue_outgoing_packet_test();
	network_manager_pop_outgoing_net_msg_test();
	network_manager_add_incoming_fragment_test();
	network_manager_pop_incoming_packet_test();
	network_manager_enqueue_outgoing_fragment_test();
	return 0;
}
