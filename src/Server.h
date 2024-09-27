#ifndef WT_SERVER_H
#define WT_SERVER_H

// This will allow strdup to be defined.
#define _POSIX_C_SOURCE 200809L

#include "HttpRequest.h"


#define WT_METHOD_ANY     0

#define WT_METHOD_GET     1
#define WT_METHOD_POST    2
#define WT_METHOD_PUT     3
#define WT_METHOD_HEAD    4
#define WT_METHOD_OPTIONS 5
#define WT_METHOD_PATCH   6
#define WT_METHOD_TRACE   7
#define WT_METHOD_CONNECT 8

int WT_init(const int port);
int WT_shutdown();

int WT_add_mapping(const char *method, const char *url, void (*handler)(HttpRequest *));
int WT_add_webpage(const char *url, const char *filepath);
int WT_add_file(const char *url, const char *filepath);
int WT_add_files(const char *path);

int WT_send_status(const int dest_fd, const int code);
int WT_send_msg(const int dest_fd, const int code, const char *msg);
int WT_send_data(const int dest_fd, const int code, const char *data, const char *dataType, const int length);
int WT_send_page(const int dest_fd, const int code, const char *filepath);
int WT_send_file(const int dest_fd, const int code, const char *filepath, const char *filetype);

void WT_set_error_file(const char *filepath);
void WT_log_error(const char *error);

#endif