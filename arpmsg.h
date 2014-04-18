#ifndef _ARPMSG_H_
#define _ARPMSG_H_

#include "common.h"
#include "unp.h"
#include "arplist.h"

#define NET_PROTO 5080
#define HARD_TYPE 1
#define PROTO_TYPE 0x0800
#define NET_ID 5081
#define HWADDR_SIZE 6
#define AR_REQ 1
#define AR_REP 2



typedef struct arp_msg{
  short int id;
  short int ar_hrd;  /*Format of the hardware */
  short int ar_pro; /*Format of the protocol*/
  char ar_hln; /*Length of the hardware address */
  char ar_pln; /*Length of protocol address */
  short int ar_op; /*ARP op code*/
  char send_ip [INET_ADDRSTRLEN];
  char send_hwaddr[6];
  char target_ip[INET_ADDRSTRLEN];
  char target_hwaddr[6];
}arp_msg;


void print_arpmsg(arp_msg *node);
void create_arpmsg(ip_hwaddr *my_info, char *target_ip, char *target_hwaddr, arp_msg *msg,int type);
int send_frame(int fd,ip_hwaddr *my_info,  char *target_ip, char *target_hwaddr,char *eth_dest,int type);
int recv_frame(int fd,arp_msg *recv);
void print_arpmsg(arp_msg *node);

#endif
