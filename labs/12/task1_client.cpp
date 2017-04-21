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

void die(char *s)
{
    perror(s);
    exit(1);
}

int main(int argc, char **argv)
{
    struct sockaddr_in si_other;
    int s, i;
    socklen_t slen=sizeof(si_other);
    char buf[BUFLEN];
    char message[BUFLEN];

    assert(argc == 3);
    int n = atoi(argv[1]);
    int m = atoi(argv[2]);

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);

    if (inet_aton(SERVER , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
          message[j] = j;
        }
        if (sendto(s, message, m, 0 , (struct sockaddr *) &si_other, slen)==-1)
        {
            die("sendto()");
        }

        //receive a reply and print it
        //clear the buffer by filling null, it might have previously received data
        memset(buf,'\0', BUFLEN);
        //try to receive some data, this is a blocking call
        if (recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_other, &slen) == -1)
        {
            die("recvfrom()");
        }

        int diff_cnt = 0;
        for (int j = 0; j < m; ++j) {
          if (buf[j] != message[j]) {
            ++diff_cnt;
          }
        }
        printf("%d bytes are different!\n", diff_cnt);


        puts(buf);
    }

    close(s);
    return 0;
}
