#include "ping.h"


void get_hardware_addr(char *ip,char *addr){

  struct sockaddr_in sa;
  hwaddr hw;
  socklen_t len=sizeof(sa);

  //printf("The target ip is %s\n",ip);

  memset(&hw,0,sizeof(hwaddr));
  hw.sll_ifindex=2;
  hw.sll_hatype=1;
  hw.sll_halen=(char)6;

  memset(&sa,0,len);


  sa.sin_family=AF_INET;
  sa.sin_port=0;

  if(inet_pton(AF_INET,ip,&sa.sin_addr)<=0){
    printf("Error with inet_pton\n");
    exit(1);
  }

  if(areq((struct sockaddr*)&sa,len,&hw)<=0){
    printf("Error getting hardware address for IP address %s\n",ip);
    exit(1);
  }

  memcpy(addr,hw.sll_addr,HADDR_SIZE);

}

int send_ping(char *dest_ip,int fd){

  struct ip iphdr;
  int ip_flags[4];
  struct icmp icmphdr;
  char src_ip[INET_ADDRSTRLEN];
  char dest_mac[6];
  char src_mac[6];
  char *buffer;
  int size=ETH_MAX_FRAME+IP_HDRLEN+ICMP_HDRLEN;
  buffer=malloc(size);
  struct sockaddr_ll addr;
  
  my_ip(src_ip);

  // IPv4 header
  // IPv4 header length (4 bits): Number of 32-bit words in header = 5
  iphdr.ip_hl = 5;
  // Internet Protocol version (4 bits): IPv4
  iphdr.ip_v = 4;
  // Type of service (8 bits)
  iphdr.ip_tos = 0;
  // Total length of datagram (16 bits): IP header + ICMP header + ICMP data
  iphdr.ip_len = htons (IP_HDRLEN+ICMP_HDRLEN);
  // ID sequence number (16 bits): unused, since single datagram
  iphdr.ip_id = htons (0);
  // Flags, and Fragmentation offset (3, 13 bits): 0 since single datagram
  // Zero (1 bit)
  ip_flags[0] = 0;
  // Do not fragment flag (1 bit)
  ip_flags[1] = 0;
  // More fragments following flag (1 bit)
  ip_flags[2] = 0;
  // Fragmentation offset (13 bits)
  ip_flags[3] = 0;
  iphdr.ip_off = htons ((ip_flags[0] << 15)
			+ (ip_flags[1] << 14)
			+ (ip_flags[2] << 13)
			+  ip_flags[3]);
  // Time-to-Live (8 bits): default to maximum value
  iphdr.ip_ttl = 255;
  // Transport layer protocol (8 bits): 1 for ICMP
  iphdr.ip_p = IPPROTO_ICMP;
  
  if(inet_pton(AF_INET,dest_ip,&(iphdr.ip_dst))<=0){
    printf("Error with inet_pton for destination address\n");
    exit(1);
  }

  if(inet_pton(AF_INET,src_ip,&(iphdr.ip_src))<=0){
    printf("Error with inet_pton for destination address\n");
    exit(1);
  }

  iphdr.ip_sum = 0;
  iphdr.ip_sum = checksum ((uint16_t *) &iphdr, IP_HDRLEN);

  // Message Type (8 bits): echo request
  icmphdr.icmp_type = ICMP_ECHO;
  // Message Code (8 bits): echo request
  icmphdr.icmp_code = 0;
  // Identifier (16 bits): usually pid of sending process - pick a number
  icmphdr.icmp_id = htons (5081);
  
  // Sequence Number (16 bits): starts at 0
  icmphdr.icmp_seq = htons (0);
  // ICMP header checksum (16 bits): set to 0 when calculating checksum
  //icmphdr.icmp_cksum = icmp4_checksum (icmphdr, data, datalen);
  icmphdr.icmp_cksum=checksum((uint16_t*)&icmphdr,ICMP_HDRLEN);

  get_hardware_addr(src_ip,src_mac);
  get_hardware_addr(dest_ip,dest_mac);

  memcpy(buffer,dest_mac,6);
  memcpy(buffer+6,src_mac,6);
  buffer[12]=ETH_P_IP/256;
  buffer[13]=ETH_P_IP%256;
  memcpy(buffer+ETH_MAX_FRAME,&iphdr,IP_HDRLEN);
  memcpy(buffer+ETH_MAX_FRAME+IP_HDRLEN,&icmphdr,ICMP_HDRLEN);

  bzero(&addr,sizeof(addr));

  addr.sll_family=AF_PACKET;
  addr.sll_protocol=htons(ETH_P_IP);
  addr.sll_ifindex=2; 
                                                                                
  addr.sll_addr[0]=dest_mac[0];
  addr.sll_addr[1]=dest_mac[1];
  addr.sll_addr[2]=dest_mac[2];
  addr.sll_addr[3]=dest_mac[3];
  addr.sll_addr[4]=dest_mac[4];
  addr.sll_addr[5]=dest_mac[5];
  addr.sll_addr[6]=0x00;
  addr.sll_addr[7]=0x00;
  addr.sll_halen=6;
  addr.sll_hatype=1;
  int bytes=sendto(fd,buffer,size,0,(struct sockaddr*)&addr,sizeof(addr));
  //char host[512];
  //ip_to_hostname(dest_ip,host);
  //printf("PING SENT: The number of ping bytes send is %d\n",bytes);
  return bytes;
}


int recv_ping(int fd, char *recv){

  int bytes;
  int size=IP_HDRLEN+ICMP_HDRLEN;
  char *buffer=malloc(size);
  struct sockaddr_in conn;
  int hlen;
  socklen_t len=sizeof(struct sockaddr_in);
  if((bytes=recvfrom(fd,buffer,size,0,(struct sockaddr*)&conn,&len))<0){
    printf("Error receiving ping data\n");
    return -1;
  }
  struct ip *iphdr;
  struct icmp *icmp;
  iphdr=(struct ip*)buffer;
  hlen=iphdr->ip_hl<<2;
  if(iphdr->ip_p!=IPPROTO_ICMP){
    printf("The received packet is not ICMP packet\n");
    return -1;
  }
  icmp=(struct icmp*)(buffer+hlen);
  if(icmp->icmp_type==ICMP_ECHOREPLY){
    if(ntohs(icmp->icmp_id)!=5081){
      printf("Not our echo reply %d != %d\n", ntohs(icmp->icmp_type), 5081);
      return -1;
    }
    else{
      inet_ntop(AF_INET,&(iphdr->ip_src),recv,INET_ADDRSTRLEN);
      char host[512];
      ip_to_hostname(recv,host);
      printf("PING RECVD %d bytes received from %s type=%d, code=%d, unique_id=%d\n",bytes,host,icmp->icmp_type,icmp->icmp_code,ntohs(icmp->icmp_id));
      return bytes;
    }
  } else {
    printf("That wasn't an echo reply message %d != %d\n", icmp->icmp_type, ICMP_ECHOREPLY);
    return -1;
  }
  return bytes;
}



