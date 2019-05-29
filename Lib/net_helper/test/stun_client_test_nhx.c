#include<malloc.h>
#include<assert.h>
#include<string.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<net_helper.h>
#include<net_helpers.h>
#include<stun_client.h>


void stun_client_create_test()
{
	struct stun_client *c;

	c = stun_client_create(NULL);
	assert(c);
	stun_client_destroy(&c);

	c = stun_client_create("stun_server=127.0.0.1,stun_port=5000");
	stun_client_destroy(&c);

	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void stun_client_send_test()
{
	uint8_t res;
	struct stun_client *c = NULL;
	struct nodeID *node;

	res = stun_packet_send(c, 30);
	assert(res);

	c = stun_client_create(NULL);

	res = stun_packet_send(c, -1);
	assert(res);

	node = net_helper_init("127.0.0.1", 6000, NULL);
	res = stun_packet_send(c, *((int*) node));
	assert(res==0);

	stun_client_destroy(&c);
	net_helper_deinit(node);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void stun_parse_respose_test()
{
	uint8_t res;
	struct stun_client *c = NULL;
	uint8_t buff[30];

	*(int16_t*)buff = htons(0x0101); // STUN binding resp
	*(int16_t*)(buff+20+0) = htons(0x0020); // binding attr
	*(int16_t*)(buff+20+2) = htons(8); // length
	*(int16_t*)(buff+20+6) = htons(6000^0x2112); // port
	buff[20+8] = 127;
	buff[20+9] = 0;
	buff[20+10] = 0;
	buff[20+11] = 2;

	res = stun_parse_response(c, NULL, 0);
	assert(res);

	c = stun_client_create(NULL);

	res = stun_parse_response(c, NULL, 0);
	assert(res);

	res = stun_parse_response(c, NULL, 20);
	assert(res);

	res = stun_parse_response(c, buff, 0);
	assert(res);

	res = stun_parse_response(c, buff, 30);
	assert(res==0);

	stun_client_destroy(&c);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void stun_extern_address_test()
{
	enum stun_state res;
	struct stun_client *c = NULL;
	uint8_t buff[30];
	char ip[80];
	int port;

	*(int16_t*)buff = htons(0x0101); // STUN binding resp
	*(int16_t*)(buff+20+0) = htons(0x0020); // binding attr
	*(int16_t*)(buff+20+2) = htons(8); // length
	*(int16_t*)(buff+20+6) = htons(6000^0x2112); // port
	buff[20+8] = 127^0x21;
	buff[20+9] = 0^0x12;
	buff[20+10] = 0^0xA4;
	buff[20+11] = 1^0x42;

	res = stun_extern_address(c, ip, &port);
	assert(res == UNBINDED);

	c = stun_client_create(NULL);

	res = stun_extern_address(c, ip, &port);
	assert(res == UNBINDED);

	stun_parse_response(c, buff, 30);

	res = stun_extern_address(c, ip, &port);
	assert(res == BINDED);
	assert(port == 6000);
	assert(strcmp(ip, "127.0.0.1")==0);

	stun_client_destroy(&c);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

void stun_network_test()
{
	struct stun_client *c;
	struct sockaddr_in si_other;
	int s, port, size;
	socklen_t slen;
	uint8_t buff[200];
	char ip[80];

	s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	slen = sizeof(si_other);
	c = stun_client_create(NULL);

	stun_packet_send(c, s);
	size = recvfrom(s, buff, 200, 0, (struct sockaddr *) &si_other, &slen);
	stun_parse_response(c, buff, size);

	stun_extern_address(c, ip, &port);
	printf("[INFO] IP address: %s, port %d\n", ip, port);

	stun_client_destroy(&c);
	close(s);
	fprintf(stderr,"%s successfully passed!\n",__func__);
}

int main()
{
	stun_client_create_test();
	stun_client_send_test();
	stun_parse_respose_test();
	stun_extern_address_test();
	//stun_network_test();
	return 0;
}
