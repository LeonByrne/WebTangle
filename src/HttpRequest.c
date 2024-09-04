#include "HttpRequest.h"

#include <malloc.h>
#include <string.h>

#include <unistd.h>

/**
 * @brief Corrupts string given to it but does not own it
 * 
 * @param data 
 * @param client_fd 
 * @return HttpRequest* 
 */
HttpRequest * create_request(char *data, const int client_fd)
{
  HttpRequest *this = malloc(sizeof(HttpRequest));
  char *savePtr;

  this->client_fd = client_fd;
  this->method = __strtok_r(data, " ", &savePtr);
  this->url = __strtok_r(NULL, " ", &savePtr);
  this->version = __strtok_r(NULL, " ", &savePtr);
  this->headers = __strtok_r(NULL, "\r\n\r\n", &savePtr);
  this->body = savePtr;

  return this;
}

/**
 * @brief
 * 
 * @param this 
 */
void delete_request(HttpRequest *this)
{
  // Free buffer
  // method is the start of the buffer
  free(this->method);

  close(this->client_fd);

  free(this);
}