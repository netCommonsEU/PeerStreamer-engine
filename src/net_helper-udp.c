/*
 *  Copyright (c) 2010 Luca Abeni
 *  Copyright (c) 2010 Csaba Kiraly
 *  Copyright (c) 2010 Alessandro Russo
 *
 *  This is free software; see lgpl-2.1.txt
 */

#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#else
#define _WIN32_WINNT 0x0501 /* WINNT>=0x501 (WindowsXP) for supporting getaddrinfo/freeaddrinfo.*/
#include "win32-net.h"
#endif

#include "net_helper.h"

#define MAX_MSG_SIZE 1024 * 60
enum L3PROTOCOL {IPv4, IPv6} l3 = IPv4;

struct nodeID {
  struct sockaddr_storage addr;
  int fd;
};

#ifdef _WIN32
static int inet_aton(const char *cp, struct in_addr *addr)
{
    if( cp==NULL || addr==NULL )
    {
        return(0);
    }

    addr->s_addr = inet_addr(cp);
    return (addr->s_addr == INADDR_NONE) ? 0 : 1;
}

struct iovec {                    /* Scatter/gather array items */
  void  *iov_base;              /* Starting address */
  size_t iov_len;               /* Number of bytes to transfer */
};

struct msghdr {
  void         *msg_name;       /* optional address */
  socklen_t     msg_namelen;    /* size of address */
  struct iovec *msg_iov;        /* scatter/gather array */
  size_t        msg_iovlen;     /* # elements in msg_iov */
  void         *msg_control;    /* ancillary data, see below */
  socklen_t     msg_controllen; /* ancillary data buffer len */
  int           msg_flags;      /* flags on received message */
};

#define MIN(A,B)    ((A)<(B) ? (A) : (B))
ssize_t recvmsg (int sd, struct msghdr *msg, int flags)
{
  ssize_t bytes_read;
  size_t expected_recv_size;
  ssize_t left2move;
  char *tmp_buf;
  char *tmp;
  int i;

  assert (msg->msg_iov);

  expected_recv_size = 0;
  for (i = 0; i < msg->msg_iovlen; i++)
    expected_recv_size += msg->msg_iov[i].iov_len;
  tmp_buf = malloc (expected_recv_size);
  if (!tmp_buf)
    return -1;

  left2move = bytes_read = recvfrom (sd,
                                     tmp_buf,
                                     expected_recv_size,
                                     flags,
                                     (struct sockaddr *) msg->msg_name,
                                     &msg->msg_namelen);

  for (tmp = tmp_buf, i = 0; i < msg->msg_iovlen; i++)
    {
      if (left2move <= 0)
        break;
      assert (msg->msg_iov[i].iov_base);
      memcpy (msg->msg_iov[i].iov_base,
              tmp, MIN (msg->msg_iov[i].iov_len, left2move));
      left2move -= msg->msg_iov[i].iov_len;
      tmp += msg->msg_iov[i].iov_len;
    }

  free (tmp_buf);

  return bytes_read;
}

ssize_t sendmsg (int sd, struct msghdr * msg, int flags)
{
  ssize_t bytes_send;
  size_t expected_send_size;
  size_t left2move;
  char *tmp_buf;
  char *tmp;
  int i;

  assert (msg->msg_iov);

  expected_send_size = 0;
  for (i = 0; i < msg->msg_iovlen; i++)
    expected_send_size += msg->msg_iov[i].iov_len;
  tmp_buf = malloc (expected_send_size);
  if (!tmp_buf)
    return -1;

  for (tmp = tmp_buf, left2move = expected_send_size, i = 0; i <
       msg->msg_iovlen; i++)
    {
      if (left2move <= 0)
        break;
      assert (msg->msg_iov[i].iov_base);
      memcpy (tmp,
              msg->msg_iov[i].iov_base,
              MIN (msg->msg_iov[i].iov_len, left2move));
      left2move -= msg->msg_iov[i].iov_len;
      tmp += msg->msg_iov[i].iov_len;
    }

  bytes_send = sendto (sd,
                       tmp_buf,
                       expected_send_size,
                       flags,
                       (struct sockaddr *) msg->msg_name, msg->msg_namelen);

  free (tmp_buf);

  return bytes_send;
}

#endif

int wait4data(const struct nodeID *s, struct timeval *tout, int *user_fds)
/* returns 0 if timeout expires 
 * returns -1 in case of error of the select function
 * retruns 1 if the nodeID file descriptor is ready to be read
 * 					(i.e., some data is ready from the network socket)
 * returns 2 if some of the user_fds file descriptors is ready
 */
{
  fd_set fds;
  int i, res, max_fd;

  FD_ZERO(&fds);
  if (s) {
    max_fd = s->fd;
    FD_SET(s->fd, &fds);
  } else {
    max_fd = -1;
  }
  if (user_fds) {
    for (i = 0; user_fds[i] != -1; i++) {
      FD_SET(user_fds[i], &fds);
      if (user_fds[i] > max_fd) {
        max_fd = user_fds[i];
      }
    }
  }
  res = select(max_fd + 1, &fds, NULL, NULL, tout);
  if (res <= 0) {
    return res;
  }
  if (s && FD_ISSET(s->fd, &fds)) {
    return 1;
  }

  /* If execution arrives here, user_fds cannot be 0
     (an FD is ready, and it's not s->fd) */
  for (i = 0; user_fds[i] != -1; i++) {
    if (!FD_ISSET(user_fds[i], &fds)) {
      user_fds[i] = -2;
    }
  }

  return 2;
}

struct nodeID *create_node(const char *IPaddr, int port)
{
  struct nodeID *s;
  int res;
  struct addrinfo hints, *result;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_NUMERICHOST;

  s = malloc(sizeof(struct nodeID));
  memset(s, 0, sizeof(struct nodeID));

  if ((res = getaddrinfo(IPaddr, NULL, &hints, &result)))
  {
    fprintf(stderr, "Cannot resolve hostname '%s'\n", IPaddr);
    return NULL;
  }
  s->addr.ss_family = result->ai_family;
  switch (result->ai_family)
  {
    case (AF_INET):
      ((struct sockaddr_in *)&s->addr)->sin_port = htons(port);
      res = inet_pton (result->ai_family, IPaddr, &((struct sockaddr_in *)&s->addr)->sin_addr);
    break;
    case (AF_INET6):
      ((struct sockaddr_in6 *)&s->addr)->sin6_port = htons(port);
      res = inet_pton (result->ai_family, IPaddr, &(((struct sockaddr_in6 *) &s->addr)->sin6_addr));
    break;
    default:
      fprintf(stderr, "Cannot resolve address family %d for '%s'\n", result->ai_family, IPaddr);
      res = 0;
      break;
  }
  freeaddrinfo(result);
  if (res != 1)
  {
    fprintf(stderr, "Could not convert address '%s'\n", IPaddr);
    free(s);

    return NULL;
  }

  s->fd = -1;

  return s;
}

struct nodeID *net_helper_init(const char *my_addr, int port, const char *config)
{
  int res;
  struct nodeID *myself;

  myself = create_node(my_addr, port);
  if (myself == NULL) {
    fprintf(stderr, "Error creating my socket (%s:%d)!\n", my_addr, port);

    return NULL;
  }
  myself->fd =  socket(myself->addr.ss_family, SOCK_DGRAM, 0);
  if (myself->fd < 0) {
    free(myself);

    return NULL;
  }
//  TODO:
//  if (addr->sa_family == AF_INET6) {
//      r = setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY, &on, sizeof(on));
//  }

  fprintf(stderr, "My sock: %d\n", myself->fd);

  switch (myself->addr.ss_family)
  {
    case (AF_INET):
        res = bind(myself->fd, (struct sockaddr *)&myself->addr, sizeof(struct sockaddr_in));
    break;
    case (AF_INET6):
        res = bind(myself->fd, (struct sockaddr *)&myself->addr, sizeof(struct sockaddr_in6));
    break;
    default:
      fprintf(stderr, "Cannot resolve address family %d in bind\n", myself->addr.ss_family);
      res = 0;
    break;
  }

  if (res < 0) {
    /* bind failed: not a local address... Just close the socket! */
    close(myself->fd);
    free(myself);

    return NULL;
  }

  return myself;
}

void bind_msg_type (uint8_t msgtype)
{
}

struct my_hdr_t {
  uint8_t m_seq;
  uint8_t frag_seq;
  uint8_t frags;
} __attribute__((packed));

int send_to_peer(const struct nodeID *from, const struct nodeID *to, const uint8_t *buffer_ptr, int buffer_size)
{
  struct msghdr msg = {0};
  static struct my_hdr_t my_hdr;
  struct iovec iov[2];
  int res;

  if (buffer_size <= 0) return -1;

  iov[0].iov_base = &my_hdr;
  iov[0].iov_len = sizeof(struct my_hdr_t);
  msg.msg_name = (void *)&to->addr;
  msg.msg_namelen = sizeof(struct sockaddr_storage);
  msg.msg_iovlen = 2;
  msg.msg_iov = iov;

  my_hdr.m_seq++;
  my_hdr.frags = (buffer_size / (MAX_MSG_SIZE)) + 1;
  my_hdr.frag_seq = 0;

  do {
    iov[1].iov_base = (void *)buffer_ptr;
    if (buffer_size > MAX_MSG_SIZE) {
      iov[1].iov_len = MAX_MSG_SIZE;
    } else {
      iov[1].iov_len = buffer_size;
    }
    my_hdr.frag_seq++;

    buffer_size -= iov[1].iov_len;
    buffer_ptr += iov[1].iov_len;
    res = sendmsg(from->fd, &msg, 0);

    if (res  < 0){
      int error = errno;
      fprintf(stderr,"net-helper: sendmsg failed errno %d: %s\n", error, strerror(error));
    }
  } while (buffer_size > 0);

  return res;
}

int recv_from_peer(const struct nodeID *local, struct nodeID **remote, uint8_t *buffer_ptr, int buffer_size)
{
  int res, recv, m_seq, frag_seq;
  struct sockaddr_storage raddr;
  struct msghdr msg = {0};
  static struct my_hdr_t my_hdr;
  struct iovec iov[2];

  iov[0].iov_base = &my_hdr;
  iov[0].iov_len = sizeof(struct my_hdr_t);
  msg.msg_name = &raddr;
  msg.msg_namelen = sizeof(struct sockaddr_storage);
  msg.msg_iovlen = 2;
  msg.msg_iov = iov;

  *remote = malloc(sizeof(struct nodeID));
  if (*remote == NULL) {
    return -1;
  }
  memset(*remote, 0, sizeof(struct nodeID));

  recv = 0;
  m_seq = -1;
  frag_seq = 0;
  do {
    iov[1].iov_base = buffer_ptr;
    if (buffer_size > MAX_MSG_SIZE) {
      iov[1].iov_len = MAX_MSG_SIZE;
    } else {
      iov[1].iov_len = buffer_size;
    }
    buffer_size -= iov[1].iov_len;
    buffer_ptr += iov[1].iov_len;
    res = recvmsg(local->fd, &msg, 0);
    recv += (res - sizeof(struct my_hdr_t));
    if (m_seq != -1 && my_hdr.m_seq != m_seq) {
      return -1;
    } else {
      m_seq = my_hdr.m_seq;
    }
    if (my_hdr.frag_seq != frag_seq + 1) {
      return -1;
    } else {
     frag_seq++;
    }
  } while ((my_hdr.frag_seq < my_hdr.frags) && (buffer_size > 0));
  memcpy(&(*remote)->addr, &raddr, msg.msg_namelen);
  (*remote)->fd = -1;

  return recv;
}

int node_addr(const struct nodeID *s, char *addr, int len)
{
  int n;

	if (s && node_ip(s, addr, len)>=0)
	{
		n = snprintf(addr + strlen(addr), len - strlen(addr) - 1, ":%d", node_port(s));
	} else
		n = snprintf(addr, len , "None");

  return n;
}

struct nodeID *nodeid_dup(const struct nodeID *s)
{
  struct nodeID *res;

  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(res, s, sizeof(struct nodeID));
  }

  return res;
}

int nodeid_equal(const struct nodeID *s1, const struct nodeID *s2)
{
	return (nodeid_cmp(s1,s2) == 0);
//  return (memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_storage)) == 0);
}

int nodeid_cmp(const struct nodeID *s1, const struct nodeID *s2)
{
	char ip1[80], ip2[80];
	int port1,port2,res;

  if(s1 && s2)
  {
    port1=node_port(s1);
    port2=node_port(s2);
    node_ip(s1,ip1,80);
    node_ip(s2,ip2,80);

  //	int res = (port1^port2)|strcmp(ip1,ip2);
  //	fprintf(stderr,"Comparing %s:%d and %s:%d\n",ip1,port1,ip2,port2);
  //	fprintf(stderr,"Result: %d\n",res);
    res  = strcmp(ip1,ip2);
    if (res!=0)
      return res;
    else
      return port1-port2;
  //		return port1 == port2 ? 0 : 1;

  //  return memcmp(&s1->addr, &s2->addr, sizeof(struct sockaddr_storage));
  }
  else
    return 0;
}

int nodeid_dump(uint8_t *b, const struct nodeID *s, size_t max_write_size)
{
  if (max_write_size < sizeof(struct sockaddr_storage)) return -1;

  memcpy(b, &s->addr, sizeof(struct sockaddr_storage));

  return sizeof(struct sockaddr_storage);
}

struct nodeID *nodeid_undump(const uint8_t *b, int *len)
{
  struct nodeID *res;
  res = malloc(sizeof(struct nodeID));
  if (res != NULL) {
    memcpy(&res->addr, b, sizeof(struct sockaddr_storage));
    res->fd = -1;
  }
  *len = sizeof(struct sockaddr_storage);

  return res;
}

void nodeid_free(struct nodeID *s)
{
  free(s);
}

int node_ip(const struct nodeID *s, char *ip, int len)
{
	int res = 0;
  switch (s->addr.ss_family)
  {
    case AF_INET:
      inet_ntop(s->addr.ss_family, &((const struct sockaddr_in *)&s->addr)->sin_addr, ip, len);
      break;
    case AF_INET6:
      inet_ntop(s->addr.ss_family, &((const struct sockaddr_in6 *)&s->addr)->sin6_addr, ip, len);
      break;
    default:
			res = -1;
      break;
  }
  if (!ip) {
	  perror("inet_ntop");
		res = -1;
  }
  if (ip && res <0 && len)
    ip[0] = '\0';
  return res;
}

int node_port(const struct nodeID *s)
{
  int res;
  switch (s->addr.ss_family)
  {
    case AF_INET:
      res = ntohs(((const struct sockaddr_in *) &s->addr)->sin_port);
      break;
    case AF_INET6:
      res = ntohs(((const struct sockaddr_in6 *)&s->addr)->sin6_port);
      break;
    default:
      res = -1;
      break;
  }
  return res;
}
