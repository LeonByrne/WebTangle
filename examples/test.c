#include "../src/Server.h"

#include <stdio.h>
#include <unistd.h>

void handler_test(Server *server, HttpRequest *request)
{
  printf("Handler reached\n");
}

int main()
{
	Server server;

	int s = server_init(&server, 8080, 10);
	if(s != 0)
	{
		printf("Failed to create server. Code: %d\n", s);
	}

	add_mapping(&server, "/", handler_test);

	start_server(&server);

	sleep(1000*60);
}