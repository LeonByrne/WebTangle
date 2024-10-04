#include "Header.h"

// This will allow strdup to be defined.
#define _POSIX_C_SOURCE 200809L

#include <string.h>

/**
 * @brief Create a header object, keeps copies of the given strings
 * 
 * @param name 
 * @param value 
 * @return Header* 
 */
Header * create_header(const char *name, const char *value)
{
  Header *this = malloc(sizeof(Header));

  this->name = strdup(name);
  this->value = strdup(value);

  this->length = strlen(name) + strlen(": ") + strlen(value) + strlen("\r\n");

  return this;
}

void delete_header(Header *this)
{
  free(this->name);
  free(this->value);

  free(this);
}