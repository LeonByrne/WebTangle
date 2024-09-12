#include "PageMapping.h"

#include <string.h>
#include <malloc.h>

PageMapping * create_page_mapping(const char *url, const char *filepath)
{
  PageMapping *this = malloc(sizeof(PageMapping));

  this->url = malloc(strlen(url) + 1);
  strcpy(this->url, url);

  this->filepath = malloc(strlen(filepath) + 1);
  strcpy(this->filepath, filepath);

  return this;
}

void delete_page_mapping(PageMapping *this)
{
  free(this->url);
  free(this->filepath);

  free(this);
}