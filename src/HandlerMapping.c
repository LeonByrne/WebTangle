#include "HandlerMapping.h"

// This will allow strdup to be defined.
#define _POSIX_C_SOURCE 200809L

#include "Utils.h"

#include <malloc.h>
#include <string.h>

/**
 * @brief Create a mapping object
 * 
 * @param url 
 * @param handler 
 * @return UrlMapping* returns NULL on failure, valid pointer on success
 */
HandlerMapping * create_handler_mapping(const char *method, const char *url, void (*handler)(HttpRequest *))
{
	HandlerMapping *this = malloc(sizeof(HandlerMapping));

	// TODO make method case insensitive
	this->method = strdup(method);
	this->url = strdup(url);

	// Convert url into regex string, convert string to regex and free regex string
	char *regStr = WT_url_to_regex(url);
	if(regcomp(&this->regex, regStr, REG_EXTENDED) != 0)
	{
		delete_handler_mapping(this);
		free(regStr);
		return NULL;
	}
	free(regStr);

	// Set handler
	this->handler = handler;


	return this;
}

/**
 * @brief Frees the memory needed for a mapping
 * 
 * @param this 
 */
void delete_handler_mapping(HandlerMapping *this)
{
	free(this->method);
	free(this->url);
	regfree(&this->regex);

	free(this);
}