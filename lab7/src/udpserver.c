#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define BUFSIZE_DEFAULT 1024
#define SADDR struct sockaddr
#define SLEN sizeof(struct sockaddr_in)

int main(int argc, char **argv) {
  int port = -1;
  int bufsize = BUFSIZE_DEFAULT;

  static struct option long_options[] = {
      {"port", required_argument, 0, 'p'},
      {"bufsize", required_argument, 0, 'b'},
      {0, 0, 0, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "p:b:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'p':
        port = atoi(optarg);
        break;
      case 'b':
        bufsize = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s --port <port> [--bufsize <size>]\n", argv[0]);
        exit(1);
    }
  }

  if (port == -1) {
    fprintf(stderr, "Usage: %s --port <port> [--bufsize <size>]\n", argv[0]);
    exit(1);
  }

  int sockfd, n;
  char *mesg = malloc(bufsize);
  char ipadr[16];
  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  memset(&servaddr, 0, SLEN);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(sockfd, (SADDR *)&servaddr, SLEN) < 0) {
    perror("bind problem");
    exit(1);
  }
  printf("UDP server listening on port %d, buffer size %d\n", port, bufsize);

  while (1) {
    unsigned int len = SLEN;
    if ((n = recvfrom(sockfd, mesg, bufsize, 0, (SADDR *)&cliaddr, &len)) < 0) {
      perror("recvfrom");
      exit(1);
    }
    mesg[n] = 0;

    printf("REQUEST %s      FROM %s : %d\n", mesg,
           inet_ntop(AF_INET, (void *)&cliaddr.sin_addr.s_addr, ipadr, 16),
           ntohs(cliaddr.sin_port));

    if (sendto(sockfd, mesg, n, 0, (SADDR *)&cliaddr, len) < 0) {
      perror("sendto");
      exit(1);
    }
  }
  free(mesg);
}