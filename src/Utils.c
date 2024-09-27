#include "Utils.h"

#include <malloc.h>
#include <string.h>

/**
 * @brief Converts a url string into an appropiate string to make a regex
 * 
 * @param url 
 * @return char* 
 */
char * WT_url_to_regex(const char *url)
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