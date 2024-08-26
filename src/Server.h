#ifndef WT_SERVER_H
#define WT_SERVER_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>

#include <arpa/inet.h>

#include "UrlMapping.h"

typedef struct UrlMapping UrlMapping;

typedef struct Server
{
  int server_fd;
  struct sockaddr_in address;     // TODO could be removed, needed only for init

  bool running;
  int type;

  UrlMapping **mappings;
  int nMappings;

  int numThreads;
  pthread_t *threadPool;

  pthread_mutex_t server_lock;  // Lock to protect the data of the server
  pthread_mutex_t socketLock;   // Lock to protect the socket
} Server;

// TODO make more and less specific server_init functions
int server_init(Server *, const int, const int);

int add_mapping(Server *this, char *url, void (*handler)(Server *, HttpRequest *));
int remove_mapping(Server *this, char *url);
UrlMapping * get_mapping(Server *this, char *url);

int start_server(Server *);
int pause_server(Server *);
int resume_server(Server *);
int stop_server(Server *);

void * worker_thread(Server *);

#endif