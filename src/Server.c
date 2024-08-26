#include "Server.h"

#include <stdbool.h>
#include <malloc.h>
#include <memory.h>

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
 * @brief Adds a mapping (or overrides existing mapping) to the servers mappings
 * 
 * @param this 
 * @param url 
 * @param handler 
 * @return int 0 if new mapping added, 1 if existing mapping overriden, -1 if mapping creatino failed
 */
int add_mapping(Server *this, char *url, void (*handler)(Server *, HttpRequest *))
{
  UrlMapping *mapping = create_mapping(url, handler);
  if(mapping == NULL)
  {
    return -1;
  }

  // Check to see if url is already mapped
  for(int i = 0; i < this->nMappings; i++)
  {
    if(strcmp(this->mappings[i]->url, url) == 0)
    {
      // Url already mapped, override it
      delete_mapping(this->mappings[i]);
      this->mappings[i] = mapping;

      return 1;
    }
  }

  this->mappings = realloc(this->mappings, sizeof(UrlMapping *) * (this->nMappings + 1));
  this->mappings[this->nMappings] = mapping;
  this->nMappings++;

  return 0;
}

/**
 * @brief 
 * 
 * @param this 
 * @param url 
 * @return int 0 if successful, -1 if no such mapping existed
 */
int remove_mapping(Server *this, char *url)
{
  for(int i = 0; i < this->nMappings; i++)
  {
    if(strcmp(this->mappings[i]->url, url) == 0)
    {
      delete_mapping(this->mappings[i]);

      memcpy(this->mappings[i], this->mappings[i+1], sizeof(UrlMapping *) * (this->nMappings - i - 1));
      this->mappings = realloc(this->mappings, sizeof(UrlMapping *) * (this->nMappings - 1));
      this->nMappings--;

      return 0;
    }
  }

  return -1;
}

/**
 * @brief Get the mapping object
 * 
 * @param this 
 * @param url 
 * @return UrlMapping* NULL if no such mapping exists, valid mapping otherwise
 */
UrlMapping *get_mapping(Server *this, char *url)
{
  for(int i = 0; i < this->nMappings; i++)
  {
    if(regexec(&this->mappings[i]->regex, url, 0, NULL, 0) == 0)
    {
      return this->mappings[i];
    }
  }

  return NULL;
}

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

void * worker_thread(Server *this)
{
  char buffer[4096];
  struct pollfd pfd;
  pfd.fd = this->server_fd;
  pfd.events = POLLIN;

  while(true)
  {
    // Try to get the mutex
    pthread_mutex_lock(&this->socketLock);

    // Check must be here for speedy pause/shutdown
    // As a loop condition, each thread would have to try recv
    if(!this->running)
    {
      pthread_mutex_unlock(&this->socketLock);
      break;
    }

    // Poll for 100ms
    int p = poll(&pfd, 1, 100);
    if(p < 0)
    {
      // TODO log an error
      pthread_mutex_unlock(&this->socketLock);
      continue;
    } else if(p == 0)
    {
      // Timeout ocurred
      pthread_mutex_unlock(&this->socketLock);
      continue;
    } else if(pfd.revents & POLLIN)
    {
      // We no longer need the socket after we get the client_fd
      pthread_mutex_unlock(&this->socketLock);

      // Got data in, retrieve it
      int client_fd = accept(this->server_fd, NULL, NULL);
      recv(client_fd, buffer, 4096, 0);

      // TODO handle the data
      // printf("Recieved request: %s\n", buffer);

      HttpRequest *request = create_request(buffer, client_fd);
      UrlMapping *mapping = get_mapping(this, request->url);

      if(mapping == NULL)
      {
        printf("Can't find mapping. Url: %s is incorrect.\n", request->url);
      } else
      {
        mapping->handler(this, request);
      }
    } else
    {
      // TODO some cases could also be considered
      pthread_mutex_unlock(&this->socketLock);
    }
  }

  return NULL;
}