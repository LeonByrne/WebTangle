#ifndef WT_HTTP_RESPONSE_H
#define WT_HTTP_RESPONSE_H

// TODO can move this to another file later
typedef struct Header
{
  char *name;
  char *value;

  int length;
} Header;

Header * create_header(const char *name, const char *value);
void delete_header(Header *this);

typedef struct HttpResponse
{
  int dest_fd;
  int statusCode;

  // These are the headers
  Header **headers;
  int nHeaders;
  int headerSize;
} HttpResponse;

HttpResponse * create_response(const int dest_fd, const int statusCode);
void delete_response(HttpResponse *this);

void response_add_header(HttpResponse *this, const char *name, const char *value);
char *response_to_message(HttpResponse *this, const int statusCode, const int contentLength, const char *contentType);

#endif