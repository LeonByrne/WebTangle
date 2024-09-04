#include "Server.h"

#include <stdbool.h>
#include <malloc.h>
#include <memory.h>

#include <unistd.h>
#include <poll.h>

typedef struct RequestNode RequestNode;

typedef struct RequestNode
{
  HttpRequest *request;
  RequestNode *next;
} RequestNode;

static int server_fd;
static bool running;

static UrlMapping **mappings = NULL;
static int nMappings = 0;

static pthread_t listenThread;
static pthread_t *threadPool;
static int nThreads;

void * listen_thread(void *);
void * worker_thread(void *);

static pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;
static RequestNode *queueHead = NULL;
static RequestNode *queueTail = NULL;

void enqueue_request(HttpRequest *request);
HttpRequest * dequeue_request();

/**
 * @brief 
 * 
 * @param port 
 * @return int 
 */
int WT_init(const int port)
{
  // Setup address
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = INADDR_ANY;

  // Create socket
  server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if(server_fd == 0)
  {
    return -1;
  }

  // Bind socket
  if(bind(server_fd, (struct sockaddr *) &address, sizeof(address)) != 0)
  {
    close(server_fd);
    return -2;
  }

  // Set listen
  // TODO how big should the max queue be for listen?
  if(listen(server_fd, 32) != 0)
  {
    close(server_fd);
    return -3;
  }

  // Set running to true
  running = true;

// TODO check if threads were made successfully
  // Create listen thread
  pthread_create(&listenThread, NULL, listen_thread, NULL);

  // Create thread pool
  nThreads = 4;
  threadPool = malloc(sizeof(pthread_t) * nThreads);
  for(int i = 0; i < nThreads; i++)
  {
    pthread_create(&threadPool[i], NULL, worker_thread, NULL);
  }

  return 0;
}

int WT_shutdown()
{
  // TODO implement later

  // Set running to false

  // Stop listen thread

  // Stop worker threads

  // Free memory

  return 0;
}

int WT_add_mapping(const char *method, const char *url, void (*handler)(HttpRequest *))
{
  // TODO Choose failure codes later

  // Create mapping
  UrlMapping *mapping = create_mapping(method, url, handler);

  if(mapping == NULL)
  {
    return -1;
  }

  // Check if mapping already exists
  for(int i = 0; i < nMappings; i++)
  {
    // TODO account for different methods later
    if(strcmp(mappings[i]->url, mapping->url) == 0)
    {
      // If exists return failure
      return -1;
    }
  }

  // Else add it
  mappings = realloc(mappings, (nMappings + 1) * sizeof(UrlMapping *));
  mappings[nMappings] = mapping;
  nMappings++;

  return 0;
}

void * listen_thread(void *)
{
  // TODO setup pollfd timeout

  while(running)
  {
    // TODO pollfd before accepting

    // Accept new connection
    int client_fd = accept(server_fd, NULL, NULL);
    if(client_fd < 0)
    {
      continue;
    }

    // Allocate buffer
    char *buffer = malloc(4096);

    // Read incoming data
    int bytesRead = read(client_fd, buffer, 4096);
    if(bytesRead > 0)
    {
      HttpRequest *request = create_request(buffer, client_fd);
      enqueue_request(request);
    } else
    {
      free(buffer);
    }
  }
}

void * worker_thread(void *)
{
  while(running)
  {
    // Get a request
    HttpRequest *request = dequeue_request();

    if(request == NULL)
    {
      // May be becase of error or may no longer be running
      continue;
    }

    // TODO resolve method/url to handler and pass data along
    printf("Incoming request\n");
    printf("  url:    %s\n", request->url);
    printf("  method: %s\n", request->method);

    char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    int bytesSent = send(request->client_fd, response, strlen(response), 0);
    if(bytesSent < 0)
    {
      printf("Error sending response\n");
    }

    delete_request(request);
  }
}

void enqueue_request(HttpRequest *request)
{
  // Make new node
  RequestNode *node = malloc(sizeof(RequestNode));
  node->request = request;
  node->next = NULL;

  // Aquire lock
  pthread_mutex_lock(&queueLock);

  if(queueTail == NULL)
  {
    // If queue is empty both head and tail are node
    queueHead = node;
    queueTail = node;
  } else
  {
    // If queue isn't empty just append to tail
    queueTail->next = node;
    queueTail = node;
  }

  // Signal that new request has arrived and release lock
  pthread_cond_signal(&queueCond);
  pthread_mutex_unlock(&queueLock);
}

HttpRequest * dequeue_request()
{
  // Aquire lock
  pthread_mutex_lock(&queueLock);

  // If head is NULL wait for cond and head to not be NULL
  while(queueHead == NULL)
  {
    // Sometimes a signal might come but another thread might snatch the data first
    // So the while loop is used instead of an if, it checks for NULL one last time
    pthread_cond_wait(&queueCond, &queueLock);
  }

  // Get the head and request
  RequestNode *node = queueHead;
  HttpRequest *request = node->request;

  // Move head
  queueHead = queueHead->next;
  if(queueHead == NULL)
  {
    queueTail = NULL;
  }

  // Free memory
  free(node);

  // Release the lock
  pthread_mutex_unlock(&queueLock);

  return request;
}

// int server_init(Server *server, const int port, const int queueSize)
// {
//   if(pthread_mutex_init(&server->socketLock, NULL) != 0)
//   {
//     return -1;
//   }

//   server->address.sin_family = AF_INET;
//   server->address.sin_port = htons(port);
//   server->address.sin_addr.s_addr = INADDR_ANY;

//   server->server_fd = socket(AF_INET, SOCK_STREAM, 0);
//   if(server->server_fd == 0)
//   {
//     return -2;
//   }

//   if(bind(server->server_fd, (struct sockaddr *) &server->address, sizeof(server->address)) != 0)
//   {
//     close(server->server_fd);
//     return -3;
//   }

//   if(listen(server->server_fd, queueSize) != 0)
//   {
//     close(server->server_fd);
//     return -4;
//   }

//   server->running = false;
//   server->numThreads = 4;  // TODO make default or accept arguement
//   server->threadPool = malloc(sizeof(pthread_t) * server->numThreads);

//   return 0;
// }

// /**
//  * @brief Adds a mapping (or overrides existing mapping) to the servers mappings
//  * 
//  * @param this 
//  * @param url 
//  * @param handler 
//  * @return int 0 if new mapping added, 1 if existing mapping overriden, -1 if mapping creatino failed
//  */
// int add_mapping(Server *this, char *url, void (*handler)(Server *, HttpRequest *))
// {
//   UrlMapping *mapping = create_mapping(url, handler);
//   if(mapping == NULL)
//   {
//     return -1;
//   }

//   // Check to see if url is already mapped
//   for(int i = 0; i < this->nMappings; i++)
//   {
//     if(strcmp(this->mappings[i]->url, url) == 0)
//     {
//       // Url already mapped, override it
//       delete_mapping(this->mappings[i]);
//       this->mappings[i] = mapping;

//       return 1;
//     }
//   }

//   this->mappings = realloc(this->mappings, sizeof(UrlMapping *) * (this->nMappings + 1));
//   this->mappings[this->nMappings] = mapping;
//   this->nMappings++;

//   return 0;
// }

// /**
//  * @brief 
//  * 
//  * @param this 
//  * @param url 
//  * @return int 0 if successful, -1 if no such mapping existed
//  */
// int remove_mapping(Server *this, char *url)
// {
//   for(int i = 0; i < this->nMappings; i++)
//   {
//     if(strcmp(this->mappings[i]->url, url) == 0)
//     {
//       delete_mapping(this->mappings[i]);

//       memcpy(this->mappings[i], this->mappings[i+1], sizeof(UrlMapping *) * (this->nMappings - i - 1));
//       this->mappings = realloc(this->mappings, sizeof(UrlMapping *) * (this->nMappings - 1));
//       this->nMappings--;

//       return 0;
//     }
//   }

//   return -1;
// }

// /**
//  * @brief Get the mapping object
//  * 
//  * @param this 
//  * @param url 
//  * @return UrlMapping* NULL if no such mapping exists, valid mapping otherwise
//  */
// UrlMapping *get_mapping(Server *this, char *url)
// {
//   for(int i = 0; i < this->nMappings; i++)
//   {
//     if(regexec(&this->mappings[i]->regex, url, 0, NULL, 0) == 0)
//     {
//       return this->mappings[i];
//     }
//   }

//   return NULL;
// }

// int start_server(Server *server)
// {
//   server->running = true;

//   for(int i = 0; i < server->numThreads; i++)
//   {
//     pthread_create(&server->threadPool[i], NULL, (void * (*)(void *)) worker_thread, server);
//   }

//   return 0;
// }

// int pause_server(Server  *server)
// {
//   return 0;
// }

// int resume_server(Server *server)
// {
//   return 0;
// }

// int stop_server(Server *server)
// {
//   return 0;
// }

// void * worker_thread(Server *this)
// {
//   char buffer[4096];
//   struct pollfd pfd;
//   pfd.fd = this->server_fd;
//   pfd.events = POLLIN;

//   while(true)
//   {
//     // Try to get the mutex
//     pthread_mutex_lock(&this->socketLock);

//     // Check must be here for speedy pause/shutdown
//     // As a loop condition, each thread would have to try recv
//     if(!this->running)
//     {
//       pthread_mutex_unlock(&this->socketLock);
//       break;
//     }

//     // Poll for 100ms
//     int p = poll(&pfd, 1, 100);
//     if(p < 0)
//     {
//       // TODO log an error
//       pthread_mutex_unlock(&this->socketLock);
//       continue;
//     } else if(p == 0)
//     {
//       // Timeout ocurred
//       pthread_mutex_unlock(&this->socketLock);
//       continue;
//     } else if(pfd.revents & POLLIN)
//     {
//       // We no longer need the socket after we get the client_fd
//       pthread_mutex_unlock(&this->socketLock);

//       // Got data in, retrieve it
//       int client_fd = accept(this->server_fd, NULL, NULL);
//       recv(client_fd, buffer, 4096, 0);

//       // TODO handle the data
//       // printf("Recieved request: %s\n", buffer);

//       HttpRequest *request = create_request(buffer, client_fd);
//       UrlMapping *mapping = get_mapping(this, request->url);

//       if(mapping == NULL)
//       {
//         printf("Can't find mapping. Url: %s is incorrect.\n", request->url);
//       } else
//       {
//         mapping->handler(this, request);
//       }
//     } else
//     {
//       // TODO some cases could also be considered
//       pthread_mutex_unlock(&this->socketLock);
//     }
//   }

//   return NULL;
// }