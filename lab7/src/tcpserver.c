#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFSIZE_DEFAULT 100
#define SADDR struct sockaddr

int main(int argc, char *argv[]) {
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

  const size_t kSize = sizeof(struct sockaddr_in);
  int lfd, cfd;
  int nread;
  char *buf = malloc(bufsize);
  if (!buf) {
    perror("malloc");
    exit(1);
  }

  struct sockaddr_in servaddr;
  struct sockaddr_in cliaddr;

  if ((lfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    exit(1);
  }

  memset(&servaddr, 0, kSize);
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (bind(lfd, (SADDR *)&servaddr, kSize) < 0) {
    perror("bind");
    exit(1);
  }

  if (listen(lfd, 5) < 0) {
    perror("listen");
    exit(1);
  }

  printf("TCP server listening on port %d, buffer size %d\n", port, bufsize);

  while (1) {
    unsigned int clilen = kSize;
    if ((cfd = accept(lfd, (SADDR *)&cliaddr, &clilen)) < 0) {
      perror("accept");
      exit(1);
    }
    printf("connection established\n");

    while ((nread = read(cfd, buf, bufsize)) > 0) {
      write(1, buf, nread);
    }

    if (nread == -1) {
      perror("read");
      exit(1);
    }
    close(cfd);
  }
  free(buf);
}