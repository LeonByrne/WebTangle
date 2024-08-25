#ifndef WT_URL_MAPPING_H
#define WT_URL_MAPPING_H

#include "HttpRequest.h"
#include "Server.h"

#include <regex.h>

typedef struct UrlMapping
{
	char *url;
	regex_t regex;
	void (*handler)(Server *, HttpRequest *);
} UrlMapping;

UrlMapping * create_mapping(char *, void (*handler)(Server *, HttpRequest *));
void delete_mapping(UrlMapping *);

#endif