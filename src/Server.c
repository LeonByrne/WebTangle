#include "Server.h"

#include <stdbool.h>
#include <malloc.h>
#include <memory.h>
#include <pthread.h>

#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>

#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

typedef struct RequestNode RequestNode;

typedef struct RequestNode
{
  HttpRequest *request;
  RequestNode *next;
} RequestNode;

static int server_fd;
static bool running;

// TODO enusre cleanup is performed
static char **pageURls = NULL;
static int nPages;

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

char * status_str(const int status);

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

  // Setup mappings
  nMappings = 0;

  return 0;
}

int WT_shutdown()
{
  // Set running to false
  running = false;

  // Stop listen thread
  pthread_join(listenThread, NULL);

  // Stop worker threads
  for(int i = 0; i < nThreads; i++)
  {
    // Add a dummy (NULL) request for each worker
    enqueue_request(NULL);
  }

  for(int i = 0; i < nThreads; i++)
  {
    // Join worker threads
    pthread_join(threadPool[i], NULL);
  }

  // Free mappings
  for(int i = 0; i < nMappings; i++)
  {
    delete_mapping(mappings[i]);
  }
  free(mappings);

  // Free Request Queue, it should now contain only dummy requests
  while(queueHead != NULL)
  {
    RequestNode *node = queueHead;

    queueHead = queueHead->next;
    if(queueHead == NULL)
    {
      queueTail = NULL;
    }

    free(node);
  }

  close(server_fd);

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

int WT_add_webpages(const char *path)
{
  // TODO implement
  return 0;
}

int WT_send_status(const int dest_fd, const int code)
{
  // TODO add more status codes
  // TODO maybe move this to a new function for resue purposes
  char *statusMsg = NULL;
  if(code == 200)
  {
    statusMsg = "OK";
  } else if(code == 404)
  {
    statusMsg = "Not Found";
  } else 
  {
    statusMsg = "Error, unknown status code";
  }

  char response[256];
  int responseLength = snprintf(response, sizeof(response),
    "HTTP/1.2 %d %s\r\nContent-Length: 0\r\n\r\n",
    code,
    statusMsg
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    // TODO log error
    return -1;
  }

  return 0;
}

int WT_send_msg(const int dest_fd, const int code, const char *msg)
{
  return WT_send_data(dest_fd, code, msg, "text/plain", strlen(msg));
}

int WT_send_data(const int dest_fd, const int code, const char *data, const char *dataType, const int length)
{
  // TODO add more status codes
  // TODO maybe move this to a new function for resue purposes
  char *statusMsg = NULL;
  if(code == 200)
  {
    statusMsg = "OK";
  } else if(code == 404)
  {
    statusMsg = "Not Found";
  } else 
  {
    statusMsg = "Error, unknown status code";
  }

  // TODO does this work? Or does it need more
  char response[256];
  int responseLength = snprintf(response, sizeof(response), 
    "HTTP/1.2 %d %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
    code,
    statusMsg,
    length,
    dataType   
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    // TODO log error
    return -1;
  } 
  
  if(send(dest_fd, data, length, 0) == -1)
  {
    // TODO log error
    return -1;
  }

  return 0;
}

int WT_send_page(const int dest_fd, const int code, const char *filepath)
{
  return WT_send_file(dest_fd, code, filepath, "text/html");
}

int WT_send_file(const int dest_fd, const int code, const char *filepath, const char *filetype)
{
  // TODO add more status codes
  // TODO maybe move this to a new function for resue purposes
  char *statusMsg = NULL;
  if(code == 200)
  {
    statusMsg = "OK";
  } else if(code == 404)
  {
    statusMsg = "Not Found";
  } else 
  {
    statusMsg = "Error, unknown status code";
  }

  // Get size of file
  int fd = open(filepath, O_RDONLY);
  struct stat fileStat;
  fstat(fd, &fileStat);

  // TODO does this work? Or does it need more
  char response[256];
  int responseLength = snprintf(response, sizeof(response), 
    "HTTP/1.2 %d %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n",
    code,
    statusMsg,
    fileStat.st_size,
    filetype  
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    close(fd);

    // TODO log error
    return -1;
  }

  if(sendfile(dest_fd, fd, NULL, fileStat.st_size) == -1)
  {
    close(fd);

    // TODO log error
    return -1;
  }

  close(fd);

  return 0;
}

void * listen_thread(void *)
{
  struct pollfd pfd;
  pfd.fd = server_fd;
  pfd.events = POLLIN;

  while(running)
  {
    int p = poll(&pfd, 1, 100);
    if(p < 0)
    {
      // TODO log error
      continue;
    } else if(p == 0)
    {
      // Timeout ocurred, try again or stop if running is false
      continue;
    }

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
      // Dummy requests may indicate server has stopped working
      continue;
    }

    // printf("Incoming request\n");
    // printf("  url:    %s\n", request->url);
    // printf("  method: %s\n", request->method);

    bool matchFound = false;
    for(int i = 0; i < nMappings; i++)
    {
      if(regexec(&mappings[i]->regex, request->url, 0, NULL, 0) == 0)
      {
        mappings[i]->handler(request);

        matchFound = true;
        break;
      }
    }

    // char *response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    // int bytesSent = send(request->client_fd, response, strlen(response), 0);
    // if(bytesSent < 0)
    // {
    //   printf("Error sending response\n");
    // }

    // WT_send_status(request->client_fd, 200);
    // WT_send_msg(request->client_fd, 200, "Hello world!");
    // WT_send_page(request->client_fd, 200, "resources/test.html");

    if(!matchFound)
    {
      WT_send_status(request->client_fd, 404);
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

char * status_str(const int status)
{
  // TODO implement
  return NULL;
}