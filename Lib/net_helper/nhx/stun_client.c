#include<stun_client.h>
#include<grapes_config.h>
#include<malloc.h>
#include<stdlib.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <string.h>

struct stun_client {
	enum stun_state state;
	char ip[INET6_ADDRSTRLEN+1];
	int port;
	struct sockaddr_in stun_addr;
};

struct stun_client *stun_client_create(const char* config)
{  //stun.l.google.com:19302
	struct stun_client *sc;
	const char *stun_srv;
	int stun_srv_port;
	struct tag * tags = NULL;

	sc = malloc(sizeof(struct stun_client));
	sc->state = UNBINDED;

	tags = grapes_config_parse(config);
	grapes_config_value_int_default(tags, "stun_port", &stun_srv_port, 19302);
	stun_srv = grapes_config_value_str_default(tags, "stun_server", "74.125.140.127");

	sc->stun_addr.sin_family = AF_INET;
	inet_pton(AF_INET, stun_srv, &(sc->stun_addr.sin_addr));
	sc->stun_addr.sin_port = htons(stun_srv_port);

	if (tags)
		free(tags);

	return sc;
}

void stun_client_destroy(struct stun_client ** c)
{
	if (c && *c)
	{
		free(*c);
		*c = NULL;
	}
}

uint8_t * stun_packet(struct stun_client *c, size_t *len)
{
	uint8_t *buff = NULL;

	if (c && len)
	{
		if (c->state == UNBINDED) // generate a request
		{
			*len = 20;
			buff = malloc(sizeof(uint8_t)*(*len));
			*(int16_t*)(buff) = htons(0x0001);  // stun_method
			*(int16_t*)(buff+2) = htons(0x0000);  // msg_length
			*(int32_t*)(buff+4) = htons((int16_t)0x2112A442);  // magic cookie

			*(int32_t*)(buff+8) = htons(rand());  // transaction ID
			*(int32_t*)(buff+12) = htons(rand());
			*(int32_t*)(buff+16) = htons(rand()); 
		} else if (c->state == BINDED) //generate an indication
		{
			*len = 20;
			buff = malloc(sizeof(uint8_t)*(*len));
			*(int16_t*)(buff) = htons(0x0011);  // stun_method
			*(int16_t*)(buff+2) = htons(0x0000);  // msg_length
			*(int32_t*)(buff+4) = htons((int16_t)0x2112A442);  // magic cookie

			*(int32_t*)(buff+8) = htons(rand());  // transaction ID
			*(int32_t*)(buff+12) = htons(rand());
			*(int32_t*)(buff+16) = htons(rand()); 
		}
	}
	return buff;
}

uint8_t stun_packet_send(struct stun_client *c, int fd)
{
	uint8_t res = 1;
	size_t len;
	uint8_t *buff;

	if (c)
	{
		buff = stun_packet(c, &len);
		if (buff)
		{
			res = sendto(fd, buff, len, MSG_CONFIRM, (struct sockaddr*)&(c->stun_addr), sizeof(c->stun_addr)) >= 0 ? 0 : 2;
			free(buff);
		}
	}
	return res;
}

uint8_t stun_parse_response(struct stun_client *c, uint8_t * buff, size_t len)
{
	int16_t type, attr_len;
	uint16_t i;
	uint8_t res = 1;

	if (c && buff && len>=2 && *(int16_t*)buff == htons(0x0101)) // it is a STUN binding resp
	{
		i = 20;
		while (i<len && res)
		{
			type = ntohs(*(int16_t*)(buff+i));
			attr_len = ntohs(*(int16_t*)(buff+i+2));
			if (type == 0x0020) // XOR-mapped-address
			{
				c->port = ntohs(*(int16_t*)(buff+i+6)) ^ 0x2112;
				sprintf(c->ip, "%d.%d.%d.%d", buff[i+8]^0x21,buff[i+9]^0x12,buff[i+10]^0xA4,buff[i+11]^0x42);
				res = 0;
				c->state = BINDED;
			}
			if (type == 0x0001) // mapped-address
			{
				c->port = ntohs(*(int16_t*)(buff+i+6));
				sprintf(c->ip, "%d.%d.%d.%d", buff[i+8],buff[i+9],buff[i+10],buff[i+11]);
				res = 0;
				c->state = BINDED;
			}
			i += (4 + attr_len);
		}
	}
	return res;
}

enum stun_state stun_extern_address(struct stun_client *c, char *ext_ip, int *ext_port)
{
	if (c && ext_ip && ext_port && c->state==BINDED)
	{
		strncpy(ext_ip, c->ip, strlen(c->ip));
		ext_ip[strlen(c->ip)] = '\0';
		*ext_port = c->port;
	}
	if (c)
		return c->state;
	return UNBINDED;
}
