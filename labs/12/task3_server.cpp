#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#include <thread>

using namespace std;

#define SERVER "127.0.0.1"
#define BUFLEN 512  //Max length of buffer
#define PORT 8888   //The port on which to send data

void die(char *s) {
  perror(s);
  exit(1);
}


struct sockaddr_in si_me, si_other;
int s, i;
socklen_t slen=sizeof(si_me);
char buf[BUFLEN];
char message[BUFLEN];

int main(int argc, char **argv) {
  if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    die("socket");
  }

  memset((char *) &si_me, 0, sizeof(si_me));

  si_me.sin_family = AF_INET;
  si_me.sin_port = htons(PORT);
  si_me.sin_addr.s_addr = htonl(INADDR_ANY);

  //bind socket to port
  if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
  {
      die("bind");
  }
  int recv_len;

  while(1) {
    printf("Waiting for data...");
    fflush(stdout);

    //try to receive some data, this is a blocking call
    if ((recv_len = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen)) == -1) {
      die("recvfrom()");
    }

    assert(recv_len == 7);
    printf("Request: %s\n", buf);
    printf("Received packet from %s:%d\n", inet_ntoa(si_other.sin_addr), ntohs(si_other.sin_port));

    //now reply the client with the same data
    memcpy(buf, "REPLY!", 7);
    if (sendto(s, buf, 7, 0, (struct sockaddr*) &si_other, slen) == -1) {
      die("sendto()");
    }
  }

  close(s);
  return 0;
}
