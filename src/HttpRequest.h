#ifndef WT_HTTP_REQUEST_H
#define WT_HTTP_REQUEST_H

typedef struct HttpRequest
{
	int client_fd;
	char *method;
	char *url;
	char *version;
	char *headers;
	char *body;
} HttpRequest;

HttpRequest * create_request(char *, const int);
void delete_request(HttpRequest *);

#endif