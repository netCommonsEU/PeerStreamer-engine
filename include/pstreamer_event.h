#ifndef __PSTREAMER_EVENT_H__
#define __PSTREAMER_EVENT_H__

#include<psinstance.h>
#include<net_helper.h>

typedef void (*fd_register_f)(void *, int, char);

int register_network_fds(const struct nodeID *s, fd_register_f func, void *handler);

int pstreamer_register_fds(const struct psinstance *psh, fd_register_f func, void *handler);

#endif
