#include "ResourceMapping.h"

// This will allow strdup to be defined.
#define _POSIX_C_SOURCE 200809L

#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

ResourceMapping *create_resource_mapping(const char * url, const char *filepath)
{
  // CHeck that file exists
  int fd = open(filepath, O_RDONLY);
  if(fd < 0)
  {
    return NULL;
  }
  close(fd);

  ResourceMapping *this = malloc(sizeof(ResourceMapping));

  this->url = strdup(url);
  this->filepath = strdup(filepath);

  // TODO move to new function?
  char *extensionPos = strrchr(filepath, '.');
  if(extensionPos == NULL)
  {
    // TODO big issue, no filetype?
    this->contentType = NULL;
  } else
  {
    extensionPos++; // No need to include the . char

    if(strcmp(extensionPos, "txt") == 0)
    {
      this->contentType = strdup("text/plain");
    }
  }

  return this;
}

void delete_resource_mapping(ResourceMapping *this)
{

}