#include "Server.h"

#include <stdbool.h>
#include <malloc.h>

#include <unistd.h>
#include <poll.h>

int server_init(Server *server, const int port, const int queueSize)
{
  if(pthread_mutex_init(&server->socketLock, NULL) != 0)
  {
    return -1;
  }

  server->address.sin_family = AF_INET;
  server->address.sin_port = htons(port);
  server->address.sin_addr.s_addr = INADDR_ANY;

  server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server->server_fd == 0)
  {
    return -2;
  }

  if(bind(server->server_fd, (struct sockaddr *) &server->address, sizeof(server->address)) != 0)
  {
    close(server->server_fd);
    return -3;
  }

  if(listen(server->server_fd, queueSize) != 0)
  {
    close(server->server_fd);
    return -4;
  }

  server->running = false;
  server->numThreads = 4;  // TODO make default or accept arguement
  server->threadPool = malloc(sizeof(pthread_t) * server->numThreads);

  return 0;
}

/**
 * @brief 
 * 
 * @param server 
 */
int start_server(Server *server)
{
  server->running = true;

  for(int i = 0; i < server->numThreads; i++)
  {
    pthread_create(&server->threadPool[i], NULL, (void * (*)(void *)) worker_thread, server);
  }

  return 0;
}

int pause_server(Server  *server)
{
  return 0;
}

int resume_server(Server *server)
{
  return 0;
}

int stop_server(Server *server)
{
  return 0;
}

void * worker_thread(Server *server)
{
  char buffer[4096];
  struct pollfd pfd;
  pfd.fd = server->server_fd;
  pfd.events = POLLIN;

  while(true)
  {
    // Try to get the mutex
    pthread_mutex_lock(&server->socketLock);

    // Check must be here for speedy pause/shutdown
    // As a loop condition, each thread would have to try recv
    if(!server->running)
    {
      pthread_mutex_unlock(&server->socketLock);
      break;
    }

    // Poll for 100ms
    int p = poll(&pfd, 1, 100);
    if(p < 0)
    {
      // TODO log an error
      pthread_mutex_unlock(&server->socketLock);
      continue;
    } else if(p == 0)
    {
      // Timeout ocurred
      pthread_mutex_unlock(&server->socketLock);
      continue;
    } else if(pfd.revents & POLLIN)
    {
      // Got data in, retrieve it
      int client_fd = accept(server->server_fd, NULL, NULL);
      recv(client_fd, buffer, 4096, 0);

      // We no longer need the socket, another thread can use it
      pthread_mutex_unlock(&server->socketLock);

      // TODO handle the data
      printf("Recieved request: %s\n", buffer);
    } else
    {
      // TODO some cases could also be considered
      pthread_mutex_unlock(&server->socketLock);
    }

    // Release the socket
  }

  return NULL;
}