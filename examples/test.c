#include "../src/Server.h"

#include <stdio.h>
#include <unistd.h>

int main()
{
	Server server;

	int s = server_init(&server, 8080, 10);
	if(s != 0)
	{
		printf("Failed to create server. Code: %d\n", s);
	}
	start_server(&server);

	sleep(1000*60);
}