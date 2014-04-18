#include "arplist.h"


local_ent * insert_into_list(local_ent *root, local_ent *node){
  local_ent *head=root;
  local_ent *temp=malloc(sizeof(local_ent));
  memcpy(temp,node,sizeof(local_ent));
  temp->next=NULL;
  if(head){
    temp->next=head;
    head=temp;
  }
  else{
    head=temp;
    head->next=NULL;
  }
  return head;
}


cache_ent * add_to_cache(cache_ent *root,cache_ent *node){
  cache_ent *head=root;
  cache_ent *temp=NULL;
  cache_ent *cursor;

  for(cursor=root;cursor!=NULL;cursor=cursor->next){

    if(strncmp(cursor->ip_info.ip,node->ip_info.ip,INET_ADDRSTRLEN)==0){
      temp=cursor;
      break;
    }
  }

  if(temp==NULL){
    temp=malloc(sizeof(cache_ent));
    memcpy(temp,node,sizeof(cache_ent));
    temp->next=NULL;
    if(head){
      temp->next=head;
      head=temp;
    }
    else{
      head=temp;
      head->next=NULL;
    }
  }
  else{
    memcpy(temp->ip_info.hw_addr,node->ip_info.hw_addr,HADDR_SIZE);
    temp->partial=node->partial;
    temp->fd=node->fd;
  }
  return head;

}

void create_cache_ent(cache_ent *node,char *hwaddr, char *ip,int ifindex,unsigned short hatype,int fd,int partial){
  memcpy(node->ip_info.ip,ip,INET_ADDRSTRLEN);
  memcpy(node->ip_info.hw_addr,hwaddr,HADDR_SIZE);
  node->sll_ifindex=ifindex;
  node->sll_hatype=hatype;
  node->fd=fd;
  node->partial=partial;
  node->last_access=time(NULL);
}

cache_ent *search_in_cache(char *ip,cache_ent *root){
  cache_ent *curr=root;
  while(curr){
    if(strncmp(curr->ip_info.ip,ip,INET_ADDRSTRLEN)==0){
      return curr;
    }
    else{
      curr=curr->next;
    }
  }
  return NULL;
}


local_ent *search_by_ip(char *ip, local_ent *root){
  local_ent *curr=root;
  
  while(curr){
    if(strncmp(curr->ip_info.ip,ip,INET_ADDRSTRLEN)==0){
      return curr;
    }
    else
      curr=curr->next;
  }
  return NULL;
}

void print_addr(char *addr){
  int  prflag = 0;
  int j = 0;
  do {
    if (addr[j] != '\0') {
      prflag = 1;
      break;
    }
  } while (++j < IF_HADDR);

  if(prflag){
    printf("HW addr = ");
    char *ptr = addr;
    j = IF_HADDR;
    do {
      printf("%.2x%s", *ptr++ & 0xff, (j == 1) ? " " : ":");
    } while (--j > 0);
  }
  printf("\n");

}



void print_iphwaddr(ip_hwaddr *node){
  printf("The IP address is %s\n",node->ip);
  print_addr(node->hw_addr);
}

void print_local_list(local_ent *root){
  local_ent *curr=root;
  printf("THE CACHE \n");
  while(curr!=NULL){
    print_iphwaddr(&(curr->ip_info));
    curr=curr->next;
  }
}


void print_cache_ent(cache_ent *node){
  char *s;
  if(node->partial==1){
    s="yes";
  }
  else{
    s="no";
  }
  print_iphwaddr(&node->ip_info);
  printf("The interface index %d\n",node->sll_ifindex);
  printf("The hardware type is %hu\n",node->sll_hatype);
  printf("The unix domain file descriptor %d\n",node->fd);
  printf("Is a partial entry? %s\n",s);
}


void print_cache(cache_ent *root){
  cache_ent *curr=root;
  while(curr){
    print_cache_ent(curr);
    curr=curr->next;
  }
}

local_ent *get_local_list(){

  struct hwa_info *hwa,*hwahead;
  struct sockaddr *sa;
  local_ent *root=NULL;
  local_ent curr;

  for(hwahead=hwa=Get_hw_addrs();hwa!=NULL;hwa=hwa->hwa_next){
    if(strcmp(hwa->if_name,"eth0")==0){
      if((sa=hwa->ip_addr)!=NULL){
	char *temp=Sock_ntop_host(sa,sizeof(*sa));
	strncpy(curr.ip_info.ip,temp,INET_ADDRSTRLEN);
      }

      memcpy(curr.ip_info.hw_addr,hwa->if_haddr,HADDR_SIZE);
      root=insert_into_list(root,&curr);
    }
  }

  free_hwa_info(hwahead);
  return root;
}


void cleanup_partial_entries(cache_ent **root){
  cache_ent *curr, *prev,*temp;

  prev=NULL;
  time_t now= time(NULL);
  curr=*root;
  while(curr){
    temp=NULL;
    time_t check=now-curr->last_access;
    if(curr->partial==1 && (check>=MAX_TIMEOUT)){
      
      if(prev==NULL){
	*root=curr->next;
      }

      else{
	prev->next=curr->next;
      }
      temp=curr;
      printf("Removing partial entry with IP address %s\n",temp->ip_info.ip);
      curr=curr->next;
      close(temp->fd);
      free(temp);
    }
    else{
      prev=curr;
      curr=curr->next;
    }

  }
  return;

}
