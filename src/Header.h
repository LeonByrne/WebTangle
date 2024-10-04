#ifndef WT_HEADER_H
#define WT_HEADER_H

typedef struct Header
{
  char *name;
  char *value;

  int length;
} Header;

Header * create_header(const char *name, const char *value);
void delete_header(Header *this);

#endif