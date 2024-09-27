#ifndef WT_PAGE_MAPPING_H
#define WT_PAGE_MAPPING_H

#include <regex.h>

typedef struct PageMapping
{
  char *url;
  char *filepath;
  regex_t regex;
} PageMapping;

PageMapping * create_page_mapping(const char *url, const char *filepath);
void delete_page_mapping(PageMapping *this);

#endif