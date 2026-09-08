#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>   // добавлено

#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "common.h"

struct Server {
  char ip[255];
  int port;
};

typedef struct {
  struct Server server;
  uint64_t begin;
  uint64_t end;
  uint64_t mod;
  uint64_t result;
  bool success;
} ClientTask;

bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }

  if (errno != 0)
    return false;

  *val = i;
  return true;
}

void* process_server(void *arg) {
  ClientTask *task = (ClientTask*)arg;
  task->success = false;
  task->result = 0;

  struct hostent *hostname = gethostbyname(task->server.ip);
  if (!hostname) {
    fprintf(stderr, "gethostbyname failed with %s\n", task->server.ip);
    return NULL;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_port = htons(task->server.port);
  server.sin_addr.s_addr = *((unsigned long *)hostname->h_addr_list[0]);

  int sck = socket(AF_INET, SOCK_STREAM, 0);
  if (sck < 0) {
    perror("socket");
    return NULL;
  }

  if (connect(sck, (struct sockaddr *)&server, sizeof(server)) < 0) {
    perror("connect");
    close(sck);
    return NULL;
  }

  char request[sizeof(uint64_t) * 3];
  memcpy(request, &task->begin, sizeof(uint64_t));
  memcpy(request + sizeof(uint64_t), &task->end, sizeof(uint64_t));
  memcpy(request + 2 * sizeof(uint64_t), &task->mod, sizeof(uint64_t));

  if (send(sck, request, sizeof(request), 0) < 0) {
    perror("send");
    close(sck);
    return NULL;
  }

  char response[sizeof(uint64_t)];
  ssize_t received = recv(sck, response, sizeof(response), 0);
  if (received != sizeof(response)) {
    perror("recv");
    close(sck);
    return NULL;
  }

  memcpy(&task->result, response, sizeof(uint64_t));
  task->success = true;
  close(sck);
  return NULL;
}

int main(int argc, char **argv) {
  uint64_t k = -1;
  uint64_t mod = -1;
  char servers[255] = {'\0'};

  while (true) {
    int current_optind = optind ? optind : 1;

    static struct option options[] = {{"k", required_argument, 0, 0},
                                      {"mod", required_argument, 0, 0},
                                      {"servers", required_argument, 0, 0},
                                      {0, 0, 0, 0}};

    int option_index = 0;
    int c = getopt_long(argc, argv, "", options, &option_index);

    if (c == -1)
      break;

    switch (c) {
    case 0: {
      switch (option_index) {
      case 0:
        ConvertStringToUI64(optarg, &k);
        break;
      case 1:
        ConvertStringToUI64(optarg, &mod);
        break;
      case 2:
        memcpy(servers, optarg, strlen(optarg));
        break;
      default:
        printf("Index %d is out of options\n", option_index);
      }
    } break;

    case '?':
      printf("Arguments error\n");
      break;
    default:
      fprintf(stderr, "getopt returned character code 0%o?\n", c);
    }
  }

  if (k == -1 || mod == -1 || !strlen(servers)) {
    fprintf(stderr, "Using: %s --k 1000 --mod 5 --servers /path/to/file\n",
            argv[0]);
    return 1;
  }


  FILE *fp = fopen(servers, "r");
  if (!fp) {
    perror("fopen servers file");
    return 1;
  }

  int servers_num = 0;
  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    if (strlen(line) > 1) servers_num++;
  }
  rewind(fp);

  struct Server *to = malloc(sizeof(struct Server) * servers_num);
  for (int i = 0; i < servers_num; i++) {
    if (fgets(line, sizeof(line), fp) == NULL) break;
    char *colon = strchr(line, ':');
    if (!colon) {
      fprintf(stderr, "Invalid server format: %s", line);
      fclose(fp);
      free(to);
      return 1;
    }
    *colon = '\0';
    strcpy(to[i].ip, line);
    to[i].port = atoi(colon + 1);
  }
  fclose(fp);


  ClientTask *tasks = malloc(sizeof(ClientTask) * servers_num);
  pthread_t *threads = malloc(sizeof(pthread_t) * servers_num);

  uint64_t chunk = k / servers_num;
  uint64_t rem = k % servers_num;
  uint64_t cur = 1;
  for (int i = 0; i < servers_num; i++) {
    tasks[i].server = to[i];
    tasks[i].mod = mod;
    tasks[i].begin = cur;
    tasks[i].end = cur + chunk - 1 + (i < rem ? 1 : 0);
    cur = tasks[i].end + 1;
    tasks[i].success = false;
  }


  for (int i = 0; i < servers_num; i++) {
    if (pthread_create(&threads[i], NULL, process_server, &tasks[i]) != 0) {
      fprintf(stderr, "pthread_create failed for server %d\n", i);
      free(to);
      free(tasks);
      free(threads);
      return 1;
    }
  }


  uint64_t answer = 1;
  bool all_ok = true;
  for (int i = 0; i < servers_num; i++) {
    pthread_join(threads[i], NULL);
    if (!tasks[i].success) {
      fprintf(stderr, "Server %s:%d failed\n", tasks[i].server.ip, tasks[i].server.port);
      all_ok = false;
      break;
    }
    answer = MultModulo(answer, tasks[i].result, mod);
  }

  if (all_ok) {
    printf("%llu! mod %llu = %llu\n", k, mod, answer);
  } else {
    printf("Calculation failed\n");
  }

  free(to);
  free(tasks);
  free(threads);

  return 0;
}