#ifndef _TOUR_H_
#define _TOUR_H_

#include "unp.h"
#include "common.h"
#include "route.h"
#include "ping.h"

int open_sockets();

void fill_ip_header(struct ip *header, char *src_ip, char *dest_ip);
int forward_tour_packet(struct route *route);

int join_mcast_group(struct route *route);
int send_mcast_message(char *message);

void handle_rt_message();
void handle_pg_message();
void handle_mc_message();
void select_loop();

#endif
