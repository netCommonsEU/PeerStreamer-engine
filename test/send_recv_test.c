#include<stdint.h>
#include<stdio.h>
#include<unistd.h>
#include<assert.h>
#include<psinstance.h>
#include<grapes_msg_types.h>

void chunkiser_send_recv_test()
{
	struct psinstance * ps1, * ps2;
	int bs_port;
	char config[80];
	int8_t res;

	ps1 = psinstance_create("iface=lo,chunkiser=dummy");
	bs_port = psinstance_port(ps1);

	sprintf(config, "iface=lo,bs_addr=127.0.0.1,bs_port=%d", bs_port);
	ps2 = psinstance_create(config);

	psinstance_network_periodic(ps2);  
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_TOPOLOGY);		// join swarm message
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_SIGNALLING);		// chunk list offer
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_NEIGHBOURHOOD);	// neighbourhood adding message

	psinstance_network_periodic(ps1);
	res = psinstance_handle_msg(ps2);
	assert(res == MSG_TYPE_TOPOLOGY);		// swarm control message
	res = psinstance_handle_msg(ps2);
	assert(res == MSG_TYPE_SIGNALLING);		// chunk list offer

	psinstance_inject_chunk(ps1);
	psinstance_network_periodic(ps1);		// send chunk


	res = psinstance_handle_msg(ps2);
	assert(res == MSG_TYPE_CHUNK);			// received injected chunk

	psinstance_destroy(&ps1);
	psinstance_destroy(&ps2);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void join_test()
{
	struct psinstance * ps1, * ps2;
	int bs_port;
	char config[80];
	int8_t res;

	ps1 = psinstance_create("iface=lo,chunkiser=dummy");
	bs_port = psinstance_port(ps1);

	sprintf(config, "iface=lo,bs_addr=127.0.0.1,bs_port=%d", bs_port);
	ps2 = psinstance_create(config);

	psinstance_network_periodic(ps2);  
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_TOPOLOGY);		// join swarm message
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_SIGNALLING);		// chunk list offer
	res = psinstance_handle_msg(ps1);
	assert(res == MSG_TYPE_NEIGHBOURHOOD);	// neighbourhood adding message

	psinstance_network_periodic(ps1);
	res = psinstance_handle_msg(ps2);
	assert(res == MSG_TYPE_TOPOLOGY);		// swarm control message
	res = psinstance_handle_msg(ps2);
	assert(res == MSG_TYPE_SIGNALLING);		// chunk list offer

	psinstance_destroy(&ps1);
	psinstance_destroy(&ps2);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	join_test();
	chunkiser_send_recv_test();
	return 0;
}
