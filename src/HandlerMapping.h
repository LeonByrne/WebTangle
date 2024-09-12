#ifndef WT_HANDLER_MAPPING_H
#define WT_HANDLER_MAPPING_H

#include "HttpRequest.h"

#include <regex.h>

typedef struct HandlerMapping
{
	char *url;
	regex_t regex;
	void (*handler)(HttpRequest *);
} HandlerMapping;

HandlerMapping * create_handler_mapping(const char *method, const char *url, void (*handler)(HttpRequest *));
void delete_handler_mapping(HandlerMapping *this);

#endif