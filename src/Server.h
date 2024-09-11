#ifndef WT_SERVER_H
#define WT_SERVER_H

#include "UrlMapping.h"

#define WT_GET     "GET"
#define WT_POST    "POST"
#define WT_PUT     "PUT"
#define WT_HEAD    "HEAD"
#define WT_OPTIONS "OPTIONS"
#define WT_PATCH   "PATCH"
#define WT_TRACE   "TRACE"
#define WT_CONNECT "CONNECT"

#define WT_ANY     "ANY"

int WT_init(const int port);
int WT_shutdown();

int WT_add_mapping(const char *method, const char *url, void (*handler)(HttpRequest *));
int WT_add_webpages(const char *path);

int WT_send_status(const int dest_fd, const int code);
int WT_send_msg(const int dest_fd, const int code, const char *msg);
int WT_send_data(const int dest_fd, const int code, const char *data, const char *dataType, const int length);
int WT_send_page(const int dest_fd, const int code, const char *filepath);
int WT_send_file(const int dest_fd, const int code, const char *filepath, const char *filetype);

#endif