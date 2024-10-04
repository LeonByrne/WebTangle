#include "Header.h"

/**
 * @brief Create a header object, takes ownership of the provided strings
 * 
 * @param name 
 * @param value 
 * @return Header* 
 */
Header * create_header(const char *name, const char *value)
{
  Header *this = malloc(sizeof(Header));

  this->name = name;
  this->value = value;

  this->length = strlen(name) + strlen(": ") + strlen(value) + strlen("\r\n");

  return this;
}

void delete_header(Header *this)
{
  free(this->name);
  free(this->value);

  free(this);
}