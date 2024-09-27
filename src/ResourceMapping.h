#ifndef WT_RESOURCE_MAPPING_H
#define WT_RESOURCE_MAPPING_H

typedef struct ResourceMapping
{
  char *url;
  char *filepath;

  char *contentType;
} ResourceMapping;

ResourceMapping *create_resource_mapping(const char * url, const char *filepath);
void delete_resource_mapping(ResourceMapping *this);


#endif