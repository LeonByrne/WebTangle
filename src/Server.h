#ifndef _SERVER_H
#define _SERVER_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <arpa/inet.h>

typedef struct 
{
  int server_fd;
  struct sockaddr_in address;     // TODO could be removed, needed only for init

  bool running;
  int type;
  void (*handler)(char *);

  pthread_mutex_t socketLock;
  atomic_int totalThreads;
  atomic_int activeThreads;
  pthread_t *threadPool;
} Server;

// TODO make more and less specific server_init functions
int server_init(Server *, const int, const int);

void start_server(Server *);

void worker_thread(Server *);

#endif