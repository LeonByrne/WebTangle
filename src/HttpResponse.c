#include "HttpResponse.h"

#include <malloc.h>
#include <string.h>

// 2xx status codes
static const char *OK = "OK";
static const char *CREATED = "Created";
static const char *ACCEPTED = "Accepted";
// 203 not used yet
static const char *NO_CONTENT = "No content";
static const char *RESET_CONTENT = "Reset Content";
static const char *PARTIAL_CONTENT = "Partial Content";
// 207 not used yet
// 208 not used yet
// 226 not used yet

// 3xx status codes
static const char *MULTIPLE_CHOICES = "Multiple Choices";
static const char *MOVED_PERMANENTLY = "Moved Permanently";
static const char *FOUND = "Found";
static const char *SEE_OTHER = "See Other";
static const char *NOT_MODIFIED = "Not Modified";
static const char *USE_PROXY = "Use Proxy";
static const char *SWITCH_PROXY = "Switch Proxy";
static const char *TEMPORARY_REDIRECT = "Temporary Redirect";
static const char *PERMANENT_REDIRECT = "Permanent Redirect";

// 4xx status codes
static const char *BAD_REQUEST = "Bad request";
static const char *UNAUTHORIZED = "Unauthorized";
static const char *PAYMENT_REQUIRED = "Payment Required";
static const char *FORBIDDEN = "Forebidden";
static const char *NOT_FOUND = "Not Found";
static const char *METHOD_NOT_ALLOWED = "Method Not Allowed";
static const char *NOT_ACCEPTABLE = "Not Acceptable";
// TODO add 407-417
static const char *IM_A_TEAPOT = "I'm A Teapot";
// TODO add 421-451

// TODO move to utils or something
/**
 * @brief Returns the appropiate status code message for
 * 
 * @param status 
 * @return const char* This pointer is owned by the function, do not call free
 */
const char * status_str(const int status)
{
  // TODO when more statuses are added include them in the space provided
  switch (status)
  {
  case 200:
    return OK;
  case 201:
    return CREATED;
  case 202:
    return ACCEPTED;
  case 203:
    return NULL;
  case 204:
    return NO_CONTENT;
  case 205:
    return RESET_CONTENT;
  case 206:
    return PARTIAL_CONTENT;
  case 207:
    return NULL;
  case 208:
    return NULL;
  case 226:
    return NULL;

  case 300:
    return MULTIPLE_CHOICES;
  case 301:
    return MOVED_PERMANENTLY;
  case 302:
    return FOUND;
  case 303:
    return SEE_OTHER;
  case 304:
    return NOT_MODIFIED;
  case 305:
    return USE_PROXY;
  case 306:
    return SWITCH_PROXY;
  case 307:
    return TEMPORARY_REDIRECT;
  case 308:
    return PERMANENT_REDIRECT;
  
  case 400:
    return BAD_REQUEST;
  case 401:
    return UNAUTHORIZED;
  case 402:
    return PAYMENT_REQUIRED;
  case 403:
    return FORBIDDEN;
  case 404:
    return NOT_FOUND;
  case 405:
    return METHOD_NOT_ALLOWED;
  case 406:
    return NOT_ACCEPTABLE;
  case 407:
    return NULL;
  case 408:
    return NULL;
  case 409:
    return NULL;
  case 410:
    return NULL;
  case 411:
    return NULL;
  case 412:
    return NULL;
  case 413:
    return NULL;
  case 414:
    return NULL;
  case 415:
    return NULL;
  case 416:
    return NULL;
  case 417:
    return NULL;
  case 418:
    return IM_A_TEAPOT;
  case 421:
    return NULL;
  case 422:
    return NULL;
  case 423:
    return NULL;
  case 424:
    return NULL;
  case 425:
    return NULL;
  case 426:
    return NULL;
  case 428:
    return NULL;
  case 429:
    return NULL;
  case 431:
    return NULL;
  case 451:
    return NULL;

  default:
    return NULL;
  }
}

HttpResponse * create_response(const int dest_fd, const int statusCode)
{
  HttpResponse *this = malloc(sizeof(HttpResponse)); 

  this->dest_fd = dest_fd;
  this->statusCode = statusCode;

  this->headers = NULL;
  this->nHeaders = 0;
  this->headerSize = 0;
}

void delete_response(HttpResponse *this)
{
  for(int i = 0; i < this->nHeaders; i++)
  {
    delete_header(this->headers[i]);
  }

  free(this->headers);
  free(this);
}

/**
 * @brief Adds a new header to the response. Takes ownership of the strings
 * 
 * @param this 
 * @param name 
 * @param value 
 */
void response_add_header(HttpResponse *this, const char *name, const char *value)
{
  Header *header = create_header(name, value);

  // The headers will be larger now
  this->headerSize += header->length;

  // Get one additinal space in the arrays
  this->headers = realloc(this->headers, sizeof(Header *) * (this->nHeaders + 1));
  this->headers[this->nHeaders] = header;
  this->nHeaders++;
}

/**
 * @brief Makes all of the response bar the body
 * 
 * @param this 
 * @return char* 
 */
char *response_to_message(HttpResponse *this, const int statusCode, const int contentLength, const char *contentType)
{
  // TODO maybe work out the message length precisely. Not needed now, eyeball it
  char *msg = malloc(256 + this->headerSize);

  // TODO implement this logic
  // Copy in the required start, no type if NULL
  int len;
  if(contentType == NULL)
  {
    // No contentType
    len = snprintf(msg, 256 + this->headerSize,
      "HTTP/1.2 %d %s\r\nContent-Length: %d\r\n",
      statusCode,
      status_str(statusCode),
      contentLength
    );
  } else
  {
    // Has a contentType
    len = snprintf(msg, 256 + this->headerSize,
      "HTTP/1.2 %d %s\r\nContent-Length: %d\r\nContent-Type: %s\r\n",
      statusCode,
      status_str(statusCode),
      contentLength,
      contentType
    );
  }

  // Copy each header
  char *headerStart = msg + len;
  for(int i = 0; i < this->nHeaders; i++)
  {
    snprintf(headerStart, this->headers[i]->length,
      "%s: %s\r\n",
      this->headers[i]->name,
      this->headers[i]->value
    );

    headerStart += this->headers[i]->length;
  }

  // Copy in the last bit to go before the body
  strcpy(headerStart, "\r\n");

  return msg;
}