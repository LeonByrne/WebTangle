#include "../src/Server.h"

#include <stdio.h>
#include <unistd.h>

void handler_test(HttpRequest *request)
{
  printf("Handler reached\n");
}

int main()
{
	int s = WT_init(8080);
	if(s != 0)
	{
		printf("Failed to create server. Code: %d\n", s);
	}

	sleep(600);

	WT_shutdown();

	printf("Server offline\n");

	return 0;
}