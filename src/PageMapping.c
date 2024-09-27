#include "PageMapping.h"

// This will allow strdup to be defined.
#define _POSIX_C_SOURCE 200809L

#include "Utils.h"

#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

PageMapping * create_page_mapping(const char *url, const char *filepath)
{
  // Check that it exists
  int fd = open(filepath, O_RDONLY);
  if(fd < 0 )
  {
    return NULL;
  }
  close(fd);

  PageMapping *this = malloc(sizeof(PageMapping));

  this->url = strdup(url);
  this->filepath = strdup(filepath);

  char *regStr = WT_url_to_regex(url);
  if(regcomp(&this->regex, regStr, REG_EXTENDED) != 0)
  {
    delete_page_mapping(this);
    free(regStr);
    return NULL;
  }
  free(regStr);

  return this;
}

void delete_page_mapping(PageMapping *this)
{
  free(this->url);
  free(this->filepath);

  regfree(&this->regex);

  free(this);
}