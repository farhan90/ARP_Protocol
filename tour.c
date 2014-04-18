#include "tour.h"



typedef struct thread_args{
  char ip[INET_ADDRSTRLEN];
  char host[512];
  int fd;

}thread_args;

int rt_fd_out;
int rt_fd_in;
int pg_fd_out;
int pg_fd_in;
int mc_fd;

char node_ip[INET_ADDRSTRLEN];
char node_name[MAXHOSTNAMELEN];
pthread_t tid;
pthread_mutex_t lock= PTHREAD_MUTEX_INITIALIZER; 
 
static void *doit (void *args){

  thread_args *var=(thread_args*)args;
  
  pthread_detach(pthread_self());
  while(1){
    printf("pinging node named %s\n", var->host);
    pthread_mutex_lock(&lock);
    send_ping(var->ip,var->fd);
    pthread_mutex_unlock(&lock);
    sleep(1);
  }
  free(args);
  return NULL;
}


struct sockaddr_in mcast_sockaddr;

int respond_to_mcast;

int open_sockets() {
  int on = 1;
  if ((rt_fd_out = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) {
    perror("couldn't get the rt out socket");
  }
  if (setsockopt(rt_fd_out, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
    perror("couldn't set rt out as IP_HDRINCL");
  }
  if ((rt_fd_in = socket(AF_INET, SOCK_RAW, TOUR_PROTO)) < 0) {
    perror("couldn't get the rt in socket");
  }
  if (setsockopt(rt_fd_in, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
    perror("couldn't set rt in as IP_HDRINCL");
  }
  if ((pg_fd_out = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_IP))) < 0) {
    perror("couldn't get the pg out socket");
  }
  if ((pg_fd_in = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0) {
    perror("couldn't get the pg in socket");
  }
  if ((mc_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("couldn't get the mc socket");
  }
  return 0;
}

void fill_ip_header(struct ip *header, char *src_ip, char *dest_ip) {
  int ip_flags[4];
  
  bzero(header, sizeof(struct ip));

  header->ip_hl = 5;
  header->ip_v = 4;
  header->ip_tos = 0;
  header->ip_len = htons(TOUR_LEN);
  header->ip_id = htons(TOUR_ID);

  ip_flags[0] = 0;
  ip_flags[1] = 0;
  ip_flags[2] = 0;
  ip_flags[3] = 0;

  header->ip_off = htons((ip_flags[0] << 15)
      + (ip_flags[1] << 14)
      + (ip_flags[2] << 13)
      +  ip_flags[3]);
  header->ip_ttl = 255;
  header->ip_p = TOUR_PROTO;
  
  if (inet_pton(AF_INET, src_ip, &(header->ip_src)) <= 0) {
    perror("inet_pton for source address\n");
  }

  if (inet_pton(AF_INET, dest_ip, &(header->ip_dst)) <= 0) {
    perror("inet_pton for destination address\n");
  }

  header->ip_sum = 0;
  header->ip_sum = checksum((uint16_t *)header, IP_HDRLEN);
}

int forward_tour_packet(struct route *route) {
  struct ip header;
  struct sockaddr_in dest;
  char buffer[TOUR_LEN];
  char dest_name[MAXHOSTNAMELEN];
  char mcast_message[MAXLINE];
  int nsent;

  if (route->index == 0 || !already_visited(route, node_ip)) {
    join_mcast_group(route);
  }

  route->index++;

  if (route->index >= route->length) {
    sprintf(mcast_message, "this is node %s, tour has ended group members please identify yourselves", node_name);
    send_mcast_message(mcast_message);
  } else {
    ip_to_hostname(route->addr[route->index], dest_name);
    printf("forwarding from %s to %s\n", node_name, dest_name);

    // Fill in the header
    fill_ip_header(&header, node_ip, route->addr[route->index]);
    memcpy(buffer, &header, IP_HDRLEN);

    // Fill in the body
    memcpy(buffer + IP_HDRLEN, route, sizeof(struct route));

    bzero(&dest, sizeof(struct sockaddr_in));
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = header.ip_dst.s_addr;

    // Send away!
    if ((nsent = sendto(rt_fd_out, buffer, TOUR_LEN, 0, (struct sockaddr *)&dest, sizeof(struct sockaddr_in))) < 0) {
      perror("send error");
    }
  }

  return 0;
}

int join_mcast_group(struct route *route) {
  socklen_t multaddr_len;

  bzero(&mcast_sockaddr, sizeof(struct sockaddr_in));
  mcast_sockaddr.sin_family = AF_INET;
  mcast_sockaddr.sin_port = htons(route->mult_port);
  if (inet_pton(AF_INET, route->mult_addr, &(mcast_sockaddr.sin_addr)) <= 0) {
    perror("inet_pton for mcast address");
  }

  multaddr_len = sizeof(mcast_sockaddr);

  if (bind(mc_fd, (struct sockaddr *)&mcast_sockaddr, multaddr_len) < 0) {
    perror("bind to mcast address in");
  }

  Mcast_join(mc_fd, (struct sockaddr *)&mcast_sockaddr, multaddr_len, NULL, 0);
  Mcast_set_loop(mc_fd, 0);

  printf("joined the mcast group with address %s and port %d\n", route->mult_addr, route->mult_port);

  return 0;
}

int send_mcast_message(char *message) {
  int nsent;

  if ((nsent = sendto(mc_fd, message, strlen(message), 0, (struct sockaddr *)&mcast_sockaddr, sizeof(mcast_sockaddr))) < 0) {
    perror("send mcast error");
  }

  printf("node %s sent multicast: %s\n", node_name, message);

  return 0;
}

void handle_rt_message() {
  struct sockaddr_in src;
  socklen_t src_len;
  char buffer[TOUR_LEN];
  struct route *route;
  struct ip *header;
  int nread;
  time_t timestamp;
  char *timestr;
  char src_host[MAXHOSTNAMELEN];
  char *src_ip;
  src_len = sizeof(struct sockaddr_in);
  if ((nread = recvfrom(rt_fd_in, buffer, TOUR_LEN, 0, (struct sockaddr *)&src, &src_len)) < 0) {
    perror("read error");
  }

  header = (struct ip *)buffer;
  if (ntohs(header->ip_id) != TOUR_ID) {
    printf("the message with id %d is not for me\n", ntohs(header->ip_id));
    return;
  }

  route = (struct route *)(buffer + IP_HDRLEN);

  src_ip = route->addr[route->index - 1];
  ip_to_hostname(src_ip, src_host);
  timestamp = time(NULL);
  timestr = ctime(&timestamp);
  timestr[strlen(timestr) - 1] = 0;
  printf("%s received source routing packet from %s\n", timestr, src_host);
  
  if (!already_came_from(route, src_ip, node_ip)) {
  
    //handling the ping in the thread
    thread_args *arg=malloc(sizeof(thread_args));
    strcpy(arg->ip,src_ip);
    strcpy(arg->host,src_host);
    arg->fd=pg_fd_out;
    pthread_create(&tid,NULL,&doit,arg);

  }



  forward_tour_packet(route);
}

void handle_pg_message() {
  char pinged_by_ip[INET_ADDRSTRLEN];
  char pinged_by_name[MAXHOSTNAMELEN];

  if (recv_ping(pg_fd_in, pinged_by_ip) > 0) {
    ip_to_hostname(pinged_by_ip, pinged_by_name);
    printf("received ping from %s\n", pinged_by_name);
  }
}

void handle_mc_message() {
  char message[MAXLINE];
  int nread;

  if ((nread = recv(mc_fd, message, MAXLINE, 0)) < 0) {
    perror("recv mcast error");
  }

  printf("node %s received multicast: %s\n", node_name, message);

  if (respond_to_mcast) {
    sprintf(message, "node %s I am a member of this group", node_name);
    respond_to_mcast = 0;
    send_mcast_message(message);
  }
}

void select_loop() {
  fd_set readset;
  int max_fd;
  int ready;
  int max1;
  //struct timeval *timeout;
  //struct timeval tv;

  while (1) {
    FD_ZERO(&readset);
    FD_SET(rt_fd_in, &readset);
    FD_SET(mc_fd, &readset);
    FD_SET(pg_fd_in, &readset);

    max1 = max(rt_fd_in, pg_fd_in);
    max_fd = max(max1, mc_fd);
    //tv.tv_sec = 5;
    // tv.tv_usec = 0;

    //timeout = respond_to_mcast == 0 ? &tv : NULL;
    ready = select(max_fd + 1, &readset, NULL, NULL, NULL);

    if (ready > 0) {
      if (FD_ISSET(rt_fd_in, &readset)) {
        handle_rt_message();
      }
      if (FD_ISSET(mc_fd, &readset)) {
        handle_mc_message();
      }
      if (FD_ISSET(pg_fd_in, &readset)) {
         handle_pg_message();
      }
    } else if (ready == 0) {
      printf("tour application exiting\n");
      exit(0);
    }
  }
}

int main(int argc, char **argv) {
  struct route route;

  respond_to_mcast = 1;

  my_ip(node_ip);
  my_hostname(node_name);

  printf("looking up ip addrs on route...\n");
  args_to_route(argc, argv, &route);

  printf("opening sockets...\n");
  open_sockets();

  if (route.length > 1) {
    printf("forwarding to first node...\n");
    forward_tour_packet(&route);
  }

  printf("entering the select loop...\n");
  select_loop();

  return 0;
}
