#include "route.h"

void args_to_route(int argc, char **argv, struct route *route) {
  int i;

  route->length = argc;
  route->index = 0;
  route->mult_port = MULT_PORT;
  strcpy(route->mult_addr, MULT_ADDR);

  bzero(route->addr, sizeof(route->addr));

  my_ip(route->addr[0]);
  for (i = 1; i < argc; i++) {
    hostname_to_ip(argv[i], route->addr[i]);
  }
}

int already_visited(struct route *route, char *ip) {
  int i;

  for (i = 0; i < route->index; i++) {
    if (strcmp(ip, route->addr[i]) == 0) {
      return 1;
    }
  }

  return 0;
}

int already_came_from(struct route *route, char *src, char *dest) {
  int i;

  for (i = 0; i < route->index - 1; i++) {
    if (strcmp(src, route->addr[i]) == 0 && strcmp(dest, route->addr[i + 1]) == 0) {
      return 1; 
    }
  }

  return 0;
}
