#ifndef _COMMON_
#define _COMMON_

#include "unp.h"
#include <linux/if_ether.h>
#include <netinet/ip.h>       // struct ip and IP_MAXPACKET (which is 65535)
#include <netinet/ip_icmp.h>  // struct icmp, ICMP_ECHO
#include <linux/if_packet.h>

#define TEMP_FORMAT "/tmp/cse533-2-XXXXXX"
#define ARP_PATH "/tmp/cse533-2-arp"
#define HADDR_SIZE 6
#define MAX_TIMEOUT 5
#define ETH_MAX_FRAME 14
#define IP_HDRLEN 20  // IPv4 header length
#define ICMP_HDRLEN 8  // ICMP header length for echo request, excludes data

//This is a generic structure 
//to store IP and hw addr
typedef struct ip_hwaddr{
  char ip [INET_ADDRSTRLEN];
  char hw_addr[6];
}ip_hwaddr;


uint16_t checksum (uint16_t *addr, int len);

void my_hostname(char *hostname);
void my_ip(char *ip);

void hostname_to_ip(char *hostname, char *ip);
void ip_to_hostname(char *ip, char *hostname);

#endif
