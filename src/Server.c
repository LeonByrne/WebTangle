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

#include "PageMapping.h"
#include "HandlerMapping.h"

typedef struct RequestNode RequestNode;

typedef struct RequestNode
{
  HttpRequest *request;
  RequestNode *next;
} RequestNode;

static const char *OK = "OK";
static const char *NOT_FOUND = "Not Found";

static int server_fd;
static bool running;

// TODO enusre cleanup is performed
static PageMapping **pageMappings = NULL;
static int nPages = 0;

static HandlerMapping **handlerMappings = NULL;
static int nMappings = 0;

static pthread_t listenThread;
static pthread_t *threadPool;
static int nThreads;

static int loggingFile = 1; // stdout

void * listen_thread(void *);
void * worker_thread(void *);

static pthread_mutex_t queueLock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t queueCond = PTHREAD_COND_INITIALIZER;
static RequestNode *queueHead = NULL;
static RequestNode *queueTail = NULL;

void enqueue_request(HttpRequest *request);
HttpRequest * dequeue_request();

const char * status_str(const int status);

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

  // Setup pages and mappings
  nPages = 0;
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
    delete_handler_mapping(handlerMappings[i]);
  }
  free(handlerMappings);

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
  HandlerMapping *mapping = create_handler_mapping(method, url, handler);

  if(mapping == NULL)
  {
    return -1;
  }

  // Check if mapping already exists
  for(int i = 0; i < nMappings; i++)
  {
    // TODO account for different methods later
    if(strcmp(handlerMappings[i]->url, mapping->url) == 0)
    {
      // If exists return failure
      char error[256];
      snprintf(error, sizeof(error), "Could not add mapping for: %s\n\tMapping for this url already exists.\n", url);
      WT_log_error(error);
      return -1;
    }
  }

  // Else add it
  handlerMappings = realloc(handlerMappings, (nMappings + 1) * sizeof(HandlerMapping *));
  handlerMappings[nMappings] = mapping;
  nMappings++;

  return 0;
}

int WT_add_webpage(const char *url, const char *filepath)
{
  // TODO choose failure codes later

  // Create mapping
  PageMapping *mapping = create_page_mapping(url, filepath);

  // Check if mapping already exists
  for(int i = 0 ; i < nPages; i ++)
  {
    if(strcmp(pageMappings[i]->url, url) == 0)
    {
      // If exists return failure
      char error[256];
      snprintf(error, sizeof(error), "Could not add mapping from: %s to: %s.\n\tMapping for this url already exists.\n", url, filepath);
      WT_log_error(error);
      return -1;
    }
  }

  // Else add it
  pageMappings = realloc(pageMappings, (nPages + 1) * sizeof(PageMapping *));
  pageMappings[nPages] = mapping;
  nPages++;

  return 0;
}

int WT_add_webpages(const char *path)
{
  // TODO implement this.
  return 0;
}

int WT_send_status(const int dest_fd, const int code)
{
  char response[256];
  int responseLength = snprintf(response, sizeof(response),
    "HTTP/1.2 %d %s\r\nContent-Length: 0\r\n\r\n",
    code,
    status_str(code)
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    WT_log_error("Failed to send status to client.\n");
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
  // TODO does this work? Or does it need more
  char response[256];
  int responseLength = snprintf(response, sizeof(response), 
    "HTTP/1.2 %d %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n\r\n",
    code,
    status_str(code),
    length,
    dataType   
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    WT_log_error("Failed to send data to client.\n\tFailed on start of message.\n");
    return -1;
  } 
  
  if(send(dest_fd, data, length, 0) == -1)
  {
    WT_log_error("Failed to send data to client.\n\tFailed on body of message.\n");
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
  // Get size of file
  int fd = open(filepath, O_RDONLY);
  struct stat fileStat;
  fstat(fd, &fileStat);

  // TODO does this work? Or does it need more
  char response[256];
  int responseLength = snprintf(response, sizeof(response), 
    "HTTP/1.2 %d %s\r\nContent-Length: %ld\r\nContent-Type: %s\r\n\r\n",
    code,
    status_str(code),
    fileStat.st_size,
    filetype  
  );

  if(send(dest_fd, response, responseLength, 0) == -1)
  {
    close(fd);

    WT_log_error("Failed to send file to client.\n\tFailed on start of message.\n");
    return -1;
  }

  if(sendfile(dest_fd, fd, NULL, fileStat.st_size) == -1)
  {
    close(fd);

    WT_log_error("Failed to send file to client.\n\tFailed on file.\n");
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
      WT_log_error("Failed to poll socket.\n");
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

    bool matchFound = false;
    for(int i = 0 ; i < nPages; i++)
    {
      if(strcmp(pageMappings[i]->url, request->url) == 0)
      {
        WT_send_page(request->client_fd, 200, pageMappings[i]->filepath);

        matchFound = true;
        break;
      }
    }

    // No need to check handler mappings if page mapping found
    if(matchFound)
    {
      delete_request(request);
      continue;
    }

    for(int i = 0; i < nMappings; i++)
    {
      if(regexec(&handlerMappings[i]->regex, request->url, 0, NULL, 0) == 0)
      {
        handlerMappings[i]->handler(request);

        matchFound = true;
        break;
      }
    }

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

/**
 * @brief Returns the appropiate status code message for
 * 
 * @param status 
 * @return const char* This pointer is owned by the function, do not call free
 */
const char * status_str(const int status)
{
  // TODO add more statuses
  if(status == 200)
  {
    return OK;
  }

  return NULL;
}

void WT_set_error_file(const char *filepath)
{
  int fd = open(filepath, O_WRONLY);
  if(fd == -1)
  {
    char error[256];
    snprintf(error, sizeof(error), "Cannot set output file to: %s\n\tCould not open file.\n", filepath);

    WT_log_error(error);
  }
}

void WT_log_error(const char *error)
{
  write(loggingFile, error, strlen(error));
}