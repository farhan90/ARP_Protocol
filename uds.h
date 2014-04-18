#ifndef _UDS_H_
#define _UDS_H_

#include "common.h"


typedef struct uds {
  int fd;
  char path[PATH_MAX];
} uds;

typedef struct hwaddr{
  int sll_ifindex;
  unsigned short sll_hatype;
  unsigned char sll_halen;
  unsigned char sll_addr[8];

}hwaddr;

typedef struct uds_message{
  char target_ip [INET_ADDRSTRLEN];
  hwaddr hwinfo;
}uds_message;

int areq(struct sockaddr *IPaddr, socklen_t len, hwaddr *hw);

uds uds_create(char *path);
int uds_send(int fd, char *message, int len);
int uds_recv(int fd, char *recv_from, char *message, int len);
int uds_destroy(uds *uds_info);
int uds_connect(uds *ufd, char*path);
void print_uds_message(uds_message *msg);
void print_hwaddr(char *addr);
#endif
