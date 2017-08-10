#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<network_manager.h>


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

int main()
{
	network_manager_create_test();
	network_manager_enqueue_outgoing_packet_test();
	network_manager_pop_outgoing_net_msg_test();
	return 0;
}
