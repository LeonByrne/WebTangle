#ifndef WT_SERVER_H
#define WT_SERVER_H

#include "UrlMapping.h"

int WT_init(const int port);
int WT_shutdown();

int WT_add_mapping(const char *method, const char *url, void (*handler)(HttpRequest *));

#endif