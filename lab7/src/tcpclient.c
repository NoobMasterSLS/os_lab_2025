#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <getopt.h>

#define BUFSIZE_DEFAULT 100
#define SADDR struct sockaddr
#define SIZE sizeof(struct sockaddr_in)

int main(int argc, char *argv[]) {
  int fd;
  int nread;
  char *buf;
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

  buf = malloc(bufsize);
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket creating");
    exit(1);
  }

  memset(&servaddr, 0, SIZE);
  servaddr.sin_family = AF_INET;

  if (inet_pton(AF_INET, ip, &servaddr.sin_addr) <= 0) {
    perror("bad address");
    exit(1);
  }

  servaddr.sin_port = htons(port);

  if (connect(fd, (SADDR *)&servaddr, SIZE) < 0) {
    perror("connect");
    exit(1);
  }

  write(1, "Input message to send\n", 22);
  while ((nread = read(0, buf, bufsize)) > 0) {
    if (write(fd, buf, nread) < 0) {
      perror("write");
      exit(1);
    }
  }

  close(fd);
  free(buf);
  exit(0);
}