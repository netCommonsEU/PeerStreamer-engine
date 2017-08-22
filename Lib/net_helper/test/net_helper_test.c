#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<time.h>
#include<net_helper.h>
#include<net_helpers.h>

void create_node_test()
{
	struct nodeID * s;

	s = create_node(NULL, -1);
	assert(s == NULL);
	s = create_node(NULL, 6000);
	assert(s == NULL);
	s = create_node("192.168.0.1", 0);
	assert(s == NULL);

	s = create_node("192.168.0.1", 6000);
	assert(s);
	nodeid_free(s);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void net_helper_init_test()
{
	struct nodeID * node;

	node = net_helper_init(NULL, 0, NULL);
	assert(node == NULL);
	node = net_helper_init("10.0.0.1", 0, NULL);
	assert(node == NULL);

	node = net_helper_init("10.0.0.1", 6000, NULL);
	assert(node == NULL);

	node = net_helper_init("127.0.0.1", 6000, NULL);
	assert(node);
	nodeid_free(node);

	node = net_helper_init("::1", 6000, NULL);
	assert(node);
	nodeid_free(node);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void nodeid_dup_test()
{
	struct nodeID * n1, * n2;

	n2 = nodeid_dup(NULL);
	assert(n2 == NULL);

	n1 = create_node("127.0.0.1", 6000);
	n2 = nodeid_dup(n1);
	assert(n2);

	nodeid_free(n1);
	nodeid_free(n2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void nodeid_equal_test()
{
	struct nodeID * n1, * n2;

	assert(nodeid_equal(NULL, NULL) == 0);

	n1 = create_node("127.0.0.1", 6000);
	assert(nodeid_equal(n1, NULL) == 0);

	assert(nodeid_equal(n1, n1));
	
	n2 = create_node("127.0.0.1", 6000);
	assert(nodeid_equal(n1, n2));

	nodeid_free(n2);
	n2 = create_node("127.0.0.1", 6001);
	assert(nodeid_equal(n1, n2) == 0);

	nodeid_free(n2);
	n2 = create_node("127.0.0.2", 6000);
	assert(nodeid_equal(n1, n2) == 0);

	nodeid_free(n1);
	nodeid_free(n2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void nodeid_cmp_test()
{
	struct nodeID * n1, * n2;

	assert(nodeid_cmp(NULL, NULL) == 0);

	n1 = create_node("127.0.0.1", 6000);
	assert(nodeid_cmp(n1, NULL) > 0);

	assert(nodeid_cmp(n1, n1) == 0);
	
	n2 = create_node("127.0.0.1", 6001);
	assert(nodeid_cmp(n1, n2) < 0);

	nodeid_free(n2);
	n2 = create_node("127.0.0.2", 6000);
	assert(nodeid_cmp(n1, n2) < 0);

	nodeid_free(n1);
	nodeid_free(n2);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void node_addr_test()
{
	struct nodeID * s = NULL;
	char buff[80];
	int res;

	res = node_addr(s, NULL, 0);
	assert(res < 0);
	res = node_addr(s, buff, 0);
	assert(res < 0);

	s = create_node("10.0.0.1", 6000);
	res = node_addr(s, buff, 0);
	assert(res < 0);
	nodeid_free(s);

	s = create_node("10.0.0.1", 6000);
	res = node_addr(s, buff, 2);
	assert(res < 0);
	nodeid_free(s);

	s = create_node("10.0.0.1", 6000);
	res = node_addr(s, buff, 80);
	assert(res > 0);
	assert(strcmp(buff, "10.0.0.1:6000") == 0);
	nodeid_free(s);

	s = create_node("::1", 6000);
	res = node_addr(s, buff, 80);
	assert(res > 0);
	assert(strcmp(buff, "::1:6000") == 0);
	nodeid_free(s);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void nodeid_dump_test()
{
	struct nodeID *s = NULL, *s2 = NULL;
	uint8_t buff[80];
	int res, len;

	res = nodeid_dump(NULL, s, 0);
	assert(res < 0);

	res = nodeid_dump(buff, s, 0);
	assert(res < 0);

	res = nodeid_dump(buff, s, 80);
	assert(res < 0);

	s = create_node("10.0.0.1", 6000);
	res = nodeid_dump(buff, s, 80);
	assert(res > 0);

	s2 = nodeid_undump(NULL, NULL);
	assert(s2 == NULL);

	s2 = nodeid_undump(buff, NULL);
	assert(s2 == NULL);

	s2 = nodeid_undump(buff, &len);
	assert(s2);
	assert(nodeid_equal(s, s2));

	nodeid_free(s);
	nodeid_free(s2);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void send_recv_test()
{
	struct nodeID * n1, *n2, *r;
	char buff[80];
	char msg[] = "ciao";
	struct timeval interval;

	n1 = net_helper_init("127.0.0.1", 6000, NULL);
	n2 = net_helper_init("127.0.0.1", 6001, NULL);
	send_to_peer(n1, n2, (uint8_t *)msg, 5);
	net_helper_periodic(n1, &interval);
	recv_from_peer(n2, &r, (uint8_t *)buff, 80);
	assert(strcmp(msg, buff) == 0);
	assert(nodeid_equal(r, n1));

	net_helper_deinit(n1);
	net_helper_deinit(n2);
	nodeid_free(r);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	create_node_test();
	net_helper_init_test();
	nodeid_dup_test();
	nodeid_equal_test();
	nodeid_cmp_test();
	node_addr_test();
	nodeid_dump_test();
	send_recv_test();
	return 0;
}
