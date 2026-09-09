#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
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
  int sockfd, n;
  char *sendline, *recvline;
  struct sockaddr_in servaddr;

  char *ip = NULL;
  int port = -1;
  int bufsize = BUFSIZE_DEFAULT;

  static struct option long_options[] = {
      {"ip", required_argument, 0, 'i'},
      {"port", required_argument, 0, 'p'},
      {"bufsize", required_argument, 0, 'b'},
      {0, 0, 0, 0}
  };

  int opt;
  while ((opt = getopt_long(argc, argv, "i:p:b:", long_options, NULL)) != -1) {
    switch (opt) {
      case 'i':
        ip = optarg;
        break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'b':
        bufsize = atoi(optarg);
        break;
      default:
        fprintf(stderr, "Usage: %s --ip <ip> --port <port> [--bufsize <size>]\n", argv[0]);
        exit(1);
    }
  }

  if (ip == NULL || port == -1) {
    fprintf(stderr, "Usage: %s --ip <ip> --port <port> [--bufsize <size>]\n", argv[0]);
    exit(1);
  }

  sendline = malloc(bufsize);
  recvline = malloc(bufsize + 1);
  if (!sendline || !recvline) {
    perror("malloc");
    exit(1);
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) < 0) {
    perror("inet_pton problem");
    exit(1);
  }
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    perror("socket problem");
    exit(1);
  }

  write(1, "Enter string\n", 13);

  while ((n = read(0, sendline, bufsize)) > 0) {
    if (sendto(sockfd, sendline, n, 0, (SADDR *)&servaddr, SLEN) == -1) {
      perror("sendto problem");
      exit(1);
    }

    if (recvfrom(sockfd, recvline, bufsize, 0, NULL, NULL) == -1) {
      perror("recvfrom problem");
      exit(1);
    }
    recvline[n] = '\0'; 

    printf("REPLY FROM SERVER= %s\n", recvline);
  }
  close(sockfd);
  free(sendline);
  free(recvline);
}