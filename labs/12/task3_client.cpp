#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to listen for incoming data

void die(char *s) {
  perror(s);
  exit(1);
}

int main(int argc, char **argv) {
  struct sockaddr_in si_me, si_other;
  int s, i , recv_len;
  socklen_t slen = sizeof(si_other);
  char buf[BUFLEN];

  //create a UDP socket
  if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
      die("socket");
  }

  int broadcast_enable = 1;
  if (setsockopt(s, SOL_SOCKET, SO_BROADCAST, (void*)&broadcast_enable, sizeof(broadcast_enable)) == -1) {
    die("broadcast");
  }

  // zero out the structure
  memset((char *) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(PORT);
  si_me.sin_addr.s_addr = inet_addr("10.150.5.255");

  memcpy(buf, "REPLY!", 7);

  //now reply the client with the same data
  if (sendto(s, buf, 7, 0, (struct sockaddr*) &si_me, slen) == -1) {
      die("sendto()");
  }

  while (1) {
    memset(buf,'\0', BUFLEN);
    //try to receive some data, this is a blocking call
    printf("Waiting for the data!\n");
    if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1) {
      die("recvfrom()");
    }
    printf("%s wrote: %s\n", inet_ntoa(si_other.sin_addr), buf);
    fflush(stdout);
  }

  close(s);
  return 0;
}
