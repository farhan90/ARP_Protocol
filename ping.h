#ifndef _PING_H_
#define _PING_H_

#include "uds.h"
#include "common.h"


void get_hardware_addr(char *ip,char *addr);
int send_ping(char *dest_ip,int fd);
int recv_ping(int fd, char *recv);

#endif
