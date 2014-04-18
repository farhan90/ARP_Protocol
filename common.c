#include "common.h"

uint16_t checksum (uint16_t *addr, int len) {
  int nleft = len;
  int sum = 0;
  uint16_t *w = addr;
  uint16_t answer = 0;
  while (nleft > 1) {
    sum += *w++;
    nleft -= sizeof (uint16_t);
  }
  if (nleft == 1) {
    *(uint8_t *) (&answer) = *(uint8_t *) w;
    sum += answer;
  }
  sum = (sum >> 16) + (sum & 0xFFFF);
  sum += (sum >> 16);
  answer = ~sum;
  return (answer);
}

void my_hostname(char *hostname) {
  if (gethostname(hostname, MAXHOSTNAMELEN) == -1) {
    perror("hostname lookup for this node failed");
  }
}

void my_ip(char *ip) {
  char hostname[MAXHOSTNAMELEN];

  my_hostname(hostname);
  hostname_to_ip(hostname, ip);
}

void hostname_to_ip(char *hostname, char *ip) {
  struct hostent *ent; 
  struct sockaddr_in addr;

  // Lookup the hostname entry
  ent = gethostbyname(hostname);
  if (ent == NULL) {
    printf("%s's ip address wasn't found\n", hostname);
    return;
  }

  // Convert the IP to presentation format
  addr.sin_addr = *((struct in_addr *)(*(ent->h_addr_list)));
  if (inet_ntop(AF_INET, &addr.sin_addr, ip, INET_ADDRSTRLEN) == NULL) {
    printf("error converting %s's ip address to presentation format\n", hostname);
    return;
  }
}

void ip_to_hostname(char *ip, char *hostname) {
  struct hostent *ent;
  struct in_addr addr;

  // Convert the IP to network format
  if (inet_pton(AF_INET, ip, &addr) == 0) {
    printf("error converting %s to network format\n", ip);
    return;
  }

  // Lookup the hostname entry
  ent = gethostbyaddr(&addr, sizeof(addr), AF_INET);
  if (ent == NULL) {
    printf("%s's hostname wasn't found\n", ip);
    return;
  }

  // Copy the hostname
  strcpy(hostname, ent->h_name);
}
