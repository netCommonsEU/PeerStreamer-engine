#ifndef __STUN_CLIENT_H__
#define __STUN_CLIENT_H__

#include<stdint.h>
#include<stddef.h>

enum stun_state {UNBINDED, BINDED};

struct stun_client;

struct stun_client *stun_client_create();

void stun_client_destroy(struct stun_client ** c);

uint8_t stun_packet_send(struct stun_client *c, int fd);

uint8_t stun_parse_response(struct stun_client *c, uint8_t * buff, size_t len);

enum stun_state stun_extern_address(struct stun_client *c, char *ext_ip, int *ext_port);


#endif
