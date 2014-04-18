#ifndef _ROUTE_H
#define _ROUTE_H

#include "unp.h"
#include "common.h"

struct route {
  int length;
  int index;
  int mult_port;
  char mult_addr[INET_ADDRSTRLEN];
  char addr[32][INET_ADDRSTRLEN];
};

#define TOUR_ID 32513
#define TOUR_PROTO 171
#define TOUR_LEN IP_HDRLEN + sizeof(struct route)

#define MULT_PORT 7873
#define MULT_ADDR "224.0.0.104"

void args_to_route(int argc, char **argv, struct route *route);
void free_route(struct route *route);

int already_visited(struct route *route, char *ip);
int already_came_from(struct route *route, char *src, char *dest);

#endif
