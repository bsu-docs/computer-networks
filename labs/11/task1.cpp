#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>

int LookupIp(char *hostname) {

}

int main(int argc, char** argv) {
  char buffer[320];
  int sockfd;
  struct addrinfo hints, *servinfo, *p;
  struct sockaddr_in *h;
  int rv;
  if (argc != 2) {
    fprintf(stderr, "Usage: info <host>\n");
    return 1;
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1] , "http" , &hints , &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }
  printf("IPv4 addresses:\n");
  for(p = servinfo; p != NULL; p = p->ai_next) {
      h = (struct sockaddr_in *) p->ai_addr;
      printf("%s\n", inet_ntop(AF_INET, &(h->sin_addr), buffer, INET_ADDRSTRLEN));
  }
  freeaddrinfo(servinfo);

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_STREAM;

  if ((rv = getaddrinfo(argv[1] , "http" , &hints , &servinfo)) != 0) {
      fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
      return 1;
  }

  printf("IPv6 addresses:\n");
  for(p = servinfo; p != NULL; p = p->ai_next) {
      h = (struct sockaddr_in *) p->ai_addr;
      printf("%s\n", inet_ntop(AF_INET6, &(h->sin_addr), buffer, INET6_ADDRSTRLEN));
  }
  return 0;
}
