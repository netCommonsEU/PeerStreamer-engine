#ifndef __PSTREAMER_EVENT_H__
#define __PSTREAMER_EVENT_H__

#include<psinstance.h>
#include<net_helpers.h>

int pstreamer_register_fds(const struct psinstance *psh, fd_register_f func, void *handler);

#endif
