#ifndef _ARPLIST_H_
#define _ARPLIST_H_

#include "hw_addrs.h"
#include "common.h"

typedef struct local_ent{
  ip_hwaddr ip_info;
  struct local_ent * next;
}local_ent;

typedef struct cache_ent{
  ip_hwaddr ip_info;
  int sll_ifindex;
  unsigned short sll_hatype;
  int fd;
  int partial;
  time_t last_access;
  struct cache_ent *next;
}cache_ent;



local_ent * insert_into_list(local_ent *root,local_ent *node); 
void print_addr(char *addr);
void print_iphwaddr(ip_hwaddr *node);
void print_local_list(local_ent *root);
local_ent *get_local_list();
local_ent *search_by_ip(char *ip, local_ent *root);
cache_ent *search_in_cache(char *ip,cache_ent *root);
void create_cache_ent(cache_ent *node,char *hwaddr, char *ip,int ifindex,unsigned short hatype,int fd,int partial);
cache_ent * add_to_cache(cache_ent *root,cache_ent *node);
void print_cache(cache_ent *root);
void print_cache_ent(cache_ent *node);
void cleanup_partial_entries(cache_ent **root);
#endif
