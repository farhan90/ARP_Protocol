#include "arpmsg.h"


void print_arpmsg(arp_msg *node){
  printf("ARP message: \n");
  printf("The sender ip is %s\n",node->send_ip);
  printf("The sender ");
  print_addr(node->send_hwaddr);
  printf("The target ip is %s\n",node->target_ip);
  printf("The target ");
  print_addr(node->target_hwaddr);
  printf("The unique id is %hu\n",ntohs(node->id));
}


int recv_frame(int fd,arp_msg *recv){
  int len;
  char src_mac[6];
  char dest_mac[6];
  struct sockaddr_ll recv_addr;
  socklen_t recv_len=sizeof(struct sockaddr_ll);
  int size=ETH_MAX_FRAME+sizeof(arp_msg);
  char *buff=malloc(size);
  len=recvfrom(fd,buff,size,MSG_TRUNC,(struct sockaddr*)&recv_addr,&recv_len);
  memcpy(recv,buff+ETH_MAX_FRAME,sizeof(arp_msg));

  memcpy(dest_mac,buff,HADDR_SIZE);
  memcpy(src_mac,buff+HWADDR_SIZE,HWADDR_SIZE);
  printf("The ethernet received source ");
  print_addr(src_mac);
  printf("The ethernet received dest ");
  print_addr(dest_mac);
  printf("The arp message received ");
  print_arpmsg(recv);

  free(buff);
  return len;
}

int send_frame(int fd,ip_hwaddr *my_info, char *target_ip, char *target_hwaddr,char *eth_dest,int type){

  int size=ETH_MAX_FRAME + sizeof(arp_msg);
  char *eth_frame=malloc(size);
  struct sockaddr_ll addr;
  arp_msg msg;
  int bytes;
  memset(eth_frame,0,size);
 
  bzero(&addr,sizeof(addr));

  addr.sll_family=AF_PACKET;
  addr.sll_protocol=htons(NET_PROTO);
  addr.sll_ifindex=2; //Only sending on eth0                                               
  addr.sll_addr[0]=eth_dest[0];
  addr.sll_addr[1]=eth_dest[1];
  addr.sll_addr[2]=eth_dest[2];
  addr.sll_addr[3]=eth_dest[3];
  addr.sll_addr[4]=eth_dest[4];
  addr.sll_addr[5]=eth_dest[5];
  addr.sll_addr[6]=0x00;
  addr.sll_addr[7]=0x00;
  addr.sll_halen=6;
  addr.sll_hatype=1;
  memcpy(eth_frame,eth_dest,6);
  memcpy(eth_frame+6,my_info->hw_addr,6);
  eth_frame[12]=NET_PROTO/256;
  eth_frame[13]=NET_PROTO%256;

  create_arpmsg(my_info, target_ip,target_hwaddr,&msg,type);

  memcpy(eth_frame+ETH_MAX_FRAME,&msg,sizeof(arp_msg));
  printf("The sent ethernet source ");
  print_addr(my_info->hw_addr);
  printf("The sent ethernet dest ");
  print_addr(eth_dest);
  printf("The sent arp msg\n");
  print_arpmsg(&msg);
  bytes=sendto(fd,eth_frame,size,0,(struct sockaddr*)&addr,sizeof(addr));
  
  free(eth_frame);
  return bytes;

}


void create_arpmsg(ip_hwaddr *my_info,char *target_ip, char *target_hwaddr, arp_msg *msg,int type){
  printf("In the create arp msg method\n");
  msg->id=htons(NET_ID);
  msg->ar_hrd=htons(HARD_TYPE);
  msg->ar_pro=htons(PROTO_TYPE);
  msg->ar_hln=(char)HWADDR_SIZE;
  msg->ar_pln=(char)INET_ADDRSTRLEN;
  msg->ar_op=htons(type);
  strncpy(msg->send_ip,my_info->ip, INET_ADDRSTRLEN);
  memcpy(msg->send_hwaddr,my_info->hw_addr,6);
  strncpy(msg->target_ip,target_ip,INET_ADDRSTRLEN);

  if(target_hwaddr!=NULL){
    memcpy(msg->target_hwaddr,target_hwaddr,6);
  }
  else{
    char temp[6]={0x00,0x00,0x00,0x00,0x00,0x00};
    memcpy(msg->target_hwaddr,temp,6);
  }
  msg->ar_op=type;
}
