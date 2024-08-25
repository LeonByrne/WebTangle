#ifndef WT_SERVER_H
#define WT_SERVER_H

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
  // TODO add a hashtable for url strings to function pointers
  

  pthread_mutex_t socketLock;
  int numThreads;
  pthread_t *threadPool;
} Server;

// TODO make more and less specific server_init functions
int server_init(Server *, const int, const int);

int start_server(Server *);
int pause_server(Server *);
int resume_server(Server *);
int stop_server(Server *);

void * worker_thread(Server *);

#endif