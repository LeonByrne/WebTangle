#include "UrlMapping.h"

#include <malloc.h>
#include <string.h>

/**
 * @brief Converts a url string into an appropiate string to make a regex
 * 
 * @param url 
 * @return char* 
 */
char * url_to_regex(char *url)
{
	// No clue why this would need a buffer this big
	// TODO work out the url to regex max size
	char buffer[strlen(url) * 2];

	// Go through string, replace path variables and add escape chars
	int len = strlen(url);
	int j = 0;
	buffer[j++] = '^'; // Regex start char
	for(int i = 0; i < len; i++)
	{
		switch(url[i])
		{
			case '/':
			case ':':
			case '.':
				// Escape character needed for special characters for regex
				buffer[j++] = '\\';
				buffer[j++] = url[i];
				break;
			case '{':
				// Replace path var with [^/]+ to match anything but /
				while(url[i] != '}' && i < len)
				{
					i++;
				}

				strcpy(&buffer[j], "[^/]+");
				j+= strlen("[^/]+");
				break;
			default:
				// Copy all other characters
				buffer[j++] = url[i];
				break;
		}
	}

	buffer[j++] = '$';
	buffer[j] = '\0';

	// Make string of right size and copy buffer to it
	char *regex = malloc(j);
	strcpy(regex, buffer);

	// Return finished regex
	return regex;
}

/**
 * @brief Create a mapping object
 * 
 * @param url 
 * @param handler 
 * @return UrlMapping* returns NULL on failure, valid pointer on success
 */
UrlMapping * create_mapping(char *method, char *url, void (*handler)(Server *, HttpRequest *))
{
	// TODO use method, add to regex. If no method given allow all methods

	UrlMapping *this = malloc(sizeof(UrlMapping));

	// Allocate space and copy url string
	this->url = malloc(strlen(url) + 1);
	strcpy(this->url, url);

	// Convert url into regex string, convert string to regex and free regex string
	char *regStr = url_to_regex(url);
	if(regcomp(&this->regex, regStr, REG_EXTENDED) != 0)
	{
		delete_mapping(this);
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
void delete_mapping(UrlMapping *this)
{
	free(this->url);
	regfree(&this->regex);

	free(this);
}