#include "ping.h"


void sig_chld(int signo){

  pid_t pid;
  int stat;

  pid=waitpid(-1,&stat,WNOHANG);

  if(WIFEXITED(stat)){
    printf("The status of the child is %i and pid is %d\n",WEXITSTATUS(stat),pid);
  }
  return;

}


int main(int argc, char *argv[]){
  // if(argc<1)
  //exit(1);
  signal(SIGCHLD,sig_chld);

  char *target_ip[4]={"130.245.156.19","130.245.156.22","130.245.156.24","130.245.156.23"};

  //printf("The target ip is %s\n",target_ip);



  int fd;
  if((fd=socket(PF_PACKET,SOCK_RAW,htons(ETH_P_IP)))<0){
    printf("Error getting pf_packet socket\n");
    exit(1);
  }

  int pid;
  int i;
  for(i=0;i<4;i++){
    if((pid=fork())<0){
      perror("fork error");
      exit(1);
    }
    if(pid==0){
      int b=send_ping(target_ip[i],fd);
      char host[512];
      ip_to_hostname(target_ip[i],host);
      printf("PING SENT: to %s of bytes %d\n",host,b);
    }
  }


 int pg;

  if((pg=socket(AF_INET,SOCK_RAW,IPPROTO_ICMP))<0){
    printf("Error getting pg socket\n");
    exit(1);
  }
  
  char recv[INET_ADDRSTRLEN];
  fd_set fset;
  struct timeval timeout;
  timeout.tv_sec=MAX_TIMEOUT;
  while(1){
    FD_ZERO(&fset);
    FD_SET(pg,&fset);

    int ready=select(pg+1,&fset,NULL,NULL,&timeout);

    if(ready>0){
      recv_ping(pg,recv);
    }
    else if(ready==0){
      printf("Select timed out\n");
      break;
    }
    else{
      perror("Select in test error");
    }
  }

  return 0;

}
