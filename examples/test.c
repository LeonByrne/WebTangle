#include "../src/Server.h"

#include <stdio.h>
#include <unistd.h>

void handler_test(HttpRequest *request)
{
  printf("Handler reached\n");

	WT_send_msg(request->client_fd, 200, "handler test");
}

void path_varibale_test(HttpRequest *request)
{
	char msg[256];
	sscanf(request->url, "/var/%s", msg);

	printf("path: %s\n", request->url);
	printf("path variable: %s\n", msg);

	WT_send_msg(request->client_fd, 200, msg);
}

int main()
{
	int s = WT_init(8080);
	if(s != 0)
	{
		printf("Failed to create server. Code: %d\n", s);
	}

	WT_add_mapping("GET", "/handler", handler_test);
	WT_add_mapping("GET", "/var/{string}", path_varibale_test);

	WT_add_webpage("/test", "resources/test.html");

	sleep(600);

	WT_shutdown();

	printf("Server offline\n");

	return 0;
}