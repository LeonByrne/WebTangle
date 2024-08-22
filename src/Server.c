#include "Server.h"

#include <stdbool.h>

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

  return 0;
}

/**
 * @brief 
 * 
 * @param server 
 */
void start_server(Server *server)
{
  // TODO make a number of threads and start them
}

void worker_thread(Server *server)
{
  char buffer[4096];

  while(server->running)
  {
    // Try to get the mutex
    pthread_mutex_lock(&server->socketLock);

    // Recieve the data
    int client_fd = accept(server->server_fd, NULL, NULL);
    recv(client_fd, buffer, 4096, 0);

    // Release the socket
    pthread_mutex_unlock(&server->socketLock);

    // Handle the data
  }
}