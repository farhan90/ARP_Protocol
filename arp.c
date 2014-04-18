#include "arp.h"

ip_hwaddr my_info;
local_ent *local_list;
cache_ent *cache;
int pfd;
uds ufd;

void reply_to_client(char *target_ip, char *hw_addr,unsigned short hatype,unsigned char halen,int index,int fd){
  int bytes;
  uds_message msg;
  memset(&msg,0,sizeof(uds_message));
  memcpy(msg.target_ip,target_ip,INET_ADDRSTRLEN);
  memcpy(msg.hwinfo.sll_addr,hw_addr,HADDR_SIZE);
  msg.hwinfo.sll_ifindex=index;
  msg.hwinfo.sll_hatype=hatype;
  msg.hwinfo.sll_halen=halen;

  bytes=uds_send(fd,(char*)&msg,sizeof(uds_message));

  if(bytes<0){
    perror("Sending hardware address to client failed\n");
    printf("The error no is %d\n",errno); 
  }

}

void handle_partial_entry(char *target_ip,ip_hwaddr *my_info){
   char eth_dest[6]={0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
   int n=send_frame(pfd,my_info,target_ip,NULL,eth_dest,AR_REQ);
   if(n<=0){
     perror("Error sending arp message");
     printf("The errno is %d\n",errno);
   }

}

void handle_uds_message(uds_message *msg, int fd){
  
  //Check if ip in the local list
  local_ent *temp=search_by_ip(msg->target_ip,local_list);
  if(temp!=NULL){
    printf("Hardware address found in local list\n");
    reply_to_client(msg->target_ip,temp->ip_info.hw_addr,msg->hwinfo.sll_hatype,msg->hwinfo.sll_halen,msg->hwinfo.sll_ifindex,fd);
  }
  else{
    printf("Hardware address not found in local list, checking in cache\n");
    cache_ent *ent=search_in_cache(msg->target_ip,cache);
    if(ent==NULL){
      cache_ent var;
      printf("Not found in cache, adding a partial entry and sending arp request\n");
      printf("The file descriptor to be added is %d \n",fd);
      create_cache_ent(&var,(char*)msg->hwinfo.sll_addr,msg->target_ip,msg->hwinfo.sll_ifindex,msg->hwinfo.sll_hatype,fd,1);
      cache= add_to_cache(cache,&var);
      //print_cache(cache);
      handle_partial_entry(msg->target_ip,&my_info);
    }
    else if(ent!=NULL && ent->partial==1){
      printf("Partial entry found in cache sending ARP REQ\n");
      handle_partial_entry(msg->target_ip,&my_info);
    }

    else{
      printf("The hardware address found in cache\n");
      reply_to_client(msg->target_ip,ent->ip_info.hw_addr,msg->hwinfo.sll_hatype,msg->hwinfo.sll_halen,msg->hwinfo.sll_ifindex,fd);
      //print_cache(cache);
    }
  }

}


void handle_arp_req(arp_msg *msg){

  local_ent *temp=search_by_ip(msg->target_ip,local_list);
  if(temp!=NULL){
    printf("The ARP REQ has reached destination\n");
    send_frame(pfd,&temp->ip_info,msg->send_ip,msg->send_hwaddr,msg->send_hwaddr,AR_REP);
  }
  //create cache ent
  cache_ent node;
  int index=2; //eth0 iterface index
  int fd=0; //We just store a fd value of zero which means it has no client waiting for it
  create_cache_ent(&node,msg->send_hwaddr,msg->send_ip,index,ntohs(msg->ar_hrd),fd,0);

  //add to cache
  cache=add_to_cache(cache,&node);
  // print_cache(cache);
}


void handle_arp_rep(arp_msg *msg){
  local_ent *temp=search_by_ip(msg->target_ip,local_list);
  if(temp!=NULL){
    printf("The ARP REP has reached the destination\n");
    cache_ent *ent=search_in_cache(msg->send_ip,cache);
    if(ent!=NULL){
      printf("Partial entry found in cache, need to update it\n");
      reply_to_client(msg->send_ip,msg->send_hwaddr,ntohs(msg->ar_hrd),msg->ar_hln,ent->sll_ifindex,ent->fd);
      close(ent->fd); //close the connection socket to client
      memcpy(ent->ip_info.hw_addr,msg->send_hwaddr,HADDR_SIZE);
      ent->partial=0;
      ent->fd=0;
      cache=add_to_cache(cache,ent);
      //print_cache(cache);
    }
    else{
      printf("There is something seriously wrong somewhere\n");
    }
  }

  else{
    cache_ent node;
    int index=2; //eth0 iterface index                                                                                                                                                           
    int fd=0; //We just store a fd value of zero which means it has no client waiting for it                                                                                                     
    create_cache_ent(&node,msg->send_hwaddr,msg->send_ip,index,ntohs(msg->ar_hrd),fd,0);

    //add to cache                                                                                                                                                                               
    cache=add_to_cache(cache,&node);
    //print_cache(cache);
  }
}


int get_pfsocket(){
  int fd= socket(AF_PACKET,SOCK_RAW,htons(NET_PROTO));
  if(fd<0){
    printf("Error in creating a pf packet socket\n");
    printf("The errno is %d\n",errno);
    exit(1);
  }
  return fd;
} 


void setup_my_info(){
  char host_ip[INET_ADDRSTRLEN];
  my_ip(host_ip);

  strncpy(my_info.ip,host_ip,INET_ADDRSTRLEN);
  local_ent *temp=search_by_ip(host_ip,local_list);
  memcpy(my_info.hw_addr,temp->ip_info.hw_addr,6);
  
}



void select_loop(){
  int recv_bytes;
  arp_msg msg;
  char recv_from[512];
  uds_message temp;
  fd_set fset;
  struct sockaddr_un remote;
  socklen_t t;
  struct timeval timeout;
  timeout.tv_sec=MAX_TIMEOUT;
  int maxfd=max(ufd.fd,pfd);
  while(1){
    FD_ZERO(&fset);
    FD_SET(ufd.fd,&fset);
    FD_SET(pfd,&fset);

    int n=select(maxfd+1,&fset,NULL,NULL,&timeout);
    if(n>0){
      cleanup_partial_entries(&cache);

        if(FD_ISSET(ufd.fd,&fset)){
          printf("Waiting for a connection\n");
          int s;
    
          t=sizeof(remote);
          if((s=accept(ufd.fd,(struct sockaddr*)&remote,&t))==-1){
              perror("Accept error\n");
              printf("The error no is %d\n",errno);
              break;
            }
          uds_recv(s,recv_from,(char*)&temp,sizeof(temp));
          handle_uds_message(&temp,s);
        }

      else if(FD_ISSET(pfd,&fset)){
        printf("PF PACKET socket is set\n");
        recv_bytes=recv_frame(pfd,&msg);                                                                                                                                                             
        printf("The number of bytes recvd %d\n",recv_bytes);                                                                                                                                         
  
        if(msg.ar_op==AR_REQ){
          handle_arp_req(&msg);
        }
        else{
          handle_arp_rep(&msg);
        }  
      }
    }
    else if(n==0){
      //printf("Select timed out, cleaning partial entries\n");
      cleanup_partial_entries(&cache);
      //print_cache(cache);
    }
    else{
      perror("select error");
      exit(1);
    }
      
  }

}





int main(){

  signal(SIGPIPE,SIG_IGN);
  //int send_bytes;
  
  local_list=get_local_list();
  print_local_list(local_list);
  setup_my_info();
  cache=NULL;
  
  pfd=get_pfsocket();
  printf("The pf_packet is %d\n",pfd);

  ufd=uds_create(ARP_PATH);
  printf("The unix domain  fd is %d\n",ufd.fd);
  char a[8];
  memset(a,0,8);

  if(listen(ufd.fd,5)==-1){
    perror("Listen error\n");
    printf("The error no is %d\n",errno);
  }
  
  select_loop();


  close(pfd);
  uds_destroy(&ufd);
  return 0;

}
