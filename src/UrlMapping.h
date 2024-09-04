#ifndef WT_URL_MAPPING_H
#define WT_URL_MAPPING_H

#include "HttpRequest.h"

#include <regex.h>

typedef struct UrlMapping
{
	char *url;
	regex_t regex;
	void (*handler)(HttpRequest *);
} UrlMapping;

UrlMapping * create_mapping(const char *method, const char *url, void (*handler)(HttpRequest *));
void delete_mapping(UrlMapping *);

#endif