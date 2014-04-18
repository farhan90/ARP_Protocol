#include "uds.h"

int areq(struct sockaddr* ipaddr, socklen_t len, hwaddr* hw){
  uds_message message,recv_msg;
  int bytes,n;
  char recv_buff[512],host[512];
  uds ufd=uds_create(NULL);
  fd_set fset;
  struct timeval timeout;
  timeout.tv_sec=MAX_TIMEOUT;
  if(ipaddr->sa_family==AF_INET){
    inet_ntop(AF_INET,&((struct sockaddr_in*)ipaddr)->sin_addr,message.target_ip,INET_ADDRSTRLEN);
    if(message.target_ip==NULL){
      printf("Inet ntop error\n");
      printf("The error number is %d\n",errno);
    }
  }
  if(uds_connect(&ufd,ARP_PATH)<0){
    exit(1);
  }
  ip_to_hostname(message.target_ip,host);
  printf("Making an areq call, target ip address is %s or %s\n",message.target_ip,host);
 
  memset(hw->sll_addr,0,8); //Need to set the hardware address zero to imply its empty
 
  memcpy(&message.hwinfo,hw,sizeof(hwaddr));
 

  bytes= uds_send(ufd.fd,(char*)&message,sizeof(message));
  if(bytes<0){
    printf("Error in making a uds send call\n");
    printf("The errno is %d\n",errno);
    return -1;
  }
  // printf("The number of bytes sent in areq call %d\n",bytes);
  while(1){
    FD_ZERO(&fset);
    FD_SET(ufd.fd,&fset);
    n=select(ufd.fd+1,&fset,NULL,NULL,&timeout);
    if(n>0){
      bytes=uds_recv(ufd.fd,recv_buff,(char *)&recv_msg,sizeof(uds_message));
      if(bytes<0){
	printf("Error in making uds recv call\n");
	printf("The errno is %d\n",errno);
	return -1;
      }
      // printf("Number of bytes received in areq call %d\n",bytes);
      print_uds_message(&recv_msg);
      memcpy(hw,&recv_msg.hwinfo,sizeof(hwaddr));
      uds_destroy(&ufd);
      return 1;
    }
    else if(n==0){
      printf("The select timed out\n");
      uds_destroy(&ufd);
      return 0;
    }
    else{
      printf("Select error\n");
      uds_destroy(&ufd);
      return -1;
    }
  }
}


int uds_connect(uds *ufd,char * path){
  struct sockaddr_un remote;
  int ret=1;
  remote.sun_family=AF_UNIX;
  strcpy(remote.sun_path,path);
  if(connect(ufd->fd,(struct sockaddr*)&remote, sizeof(remote))==-1){
    perror("Connect error\n");
    printf("The errno is %d\n",errno);
    ret=-1;
  }
  return ret;
}

uds uds_create(char *path) {
  struct sockaddr_un uds_sock;
  uds uds_info;
  int tmp_fd;
  int fd;

  if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("unix domain socket error");
  }

  uds_info.fd = fd;

  if (path == NULL) {
    strcpy(uds_info.path, TEMP_FORMAT);
    tmp_fd = mkstemp(uds_info.path);
    //printf("info: generated temp file named %s\n", uds_info.path);
    if (close(tmp_fd) == -1) {
      perror("temp file close error");
    }
  } else {
    strcpy(uds_info.path, path);
  }

  if (unlink(uds_info.path) == -1) {
    printf("warning: unlink failed for %s\n", uds_info.path);
  }

  uds_sock.sun_family = AF_UNIX;
  strcpy(uds_sock.sun_path, uds_info.path);

  if (bind(fd, (struct sockaddr *)&uds_sock, sizeof(uds_sock)) == -1) {
    perror("bind error");
  }

  return uds_info;
}

int uds_send(int fd, char *message, int len) {
  //struct sockaddr_un uds_sock;

  // uds_sock.sun_family = AF_UNIX;
  //strcpy(uds_sock.sun_path, dest_path);

  //return sendto(fd, message, len, 0, (struct sockaddr *)&uds_sock, sizeof(uds_sock));
  return send(fd,message,len,0);
}

int uds_recv(int fd, char *recv_from, char *message, int len) {
  struct sockaddr_un uds_sock;
  socklen_t sock_len;
  int rv;

  sock_len = sizeof(uds_sock);
  rv = recvfrom(fd, message, len, 0, (struct sockaddr *)&uds_sock, &sock_len);

  strcpy(recv_from, uds_sock.sun_path);

  return rv;
}

int uds_destroy(uds *uds_info) {
  int rv;
  if ((rv = unlink(uds_info->path)) == -1) {
    printf("warning: unlink failed for %s\n", uds_info->path);
  }
  close(uds_info->fd);
  return rv;
}


void print_uds_message(uds_message *msg){

  printf("The target ip address is %s  ",msg->target_ip);
  // printf("The hardware interface index is %d\n",msg->hwinfo.sll_ifindex);
  //printf("The hardware type is %hu\n",msg->hwinfo.sll_hatype);
  //printf("The hardware length is %c\n",msg->hwinfo.sll_halen);
  print_hwaddr((char*)msg->hwinfo.sll_addr);
}
 

void print_hwaddr(char *addr){
  int  prflag = 0;
  int j = 0;
  do {
    if (addr[j] != '\0') {
      prflag = 1;
      break;
    }
  } while (++j < 6);

  if(prflag){
    printf("Target HW addr = ");
    char *ptr = addr;
    j = 6;
    do {
      printf("%.2x%s", *ptr++ & 0xff, (j == 1) ? " " : ":");
    } while (--j > 0);
  }
  printf("\n");

}

