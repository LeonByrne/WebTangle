#include "Server.h"

#include <stdbool.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>

#include <arpa/inet.h>
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