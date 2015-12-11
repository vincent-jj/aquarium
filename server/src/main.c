#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/select.h>
#include "platypus.h"
#include "duck.h"

void how_many_second_since_last_time(struct Platypus* platypus, struct timeval * t1, struct timeval * real_timeout);
void empty_handler(int sig);

int main()
{
	printf("Start\n");
	signal(SIGPIPE, SIG_IGN);
	struct Platypus* platypus = platypus__create();
	if(platypus == NULL)
		return 0;
	printf("Run on port %hu\n", platypus->port);
	int err, yes = 1;
	struct sockaddr_in sock_addr_server;
	platypus->socket_server = socket(AF_INET, SOCK_STREAM, 0);
	if(platypus->socket_server < 0)
	{
		perror("socket");
		return 0;
	}
	err = setsockopt(platypus->socket_server, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if(err < 0)
	{
		perror("setsockopt");
		return 0;
	}
	sock_addr_server.sin_family = AF_INET;
	sock_addr_server.sin_port = htons(platypus->port);
	sock_addr_server.sin_addr.s_addr = INADDR_ANY;
	err = bind(platypus->socket_server, (struct sockaddr*)&sock_addr_server, sizeof(sock_addr_server));
	if(err < 0)
	{
		perror("bind");
		close(platypus->socket_server);
		return 0;
	}
	err = listen(platypus->socket_server, 10);
	if(err < 0)
	{
		perror("listen");
		close(platypus->socket_server);
		return 0;
	}
	struct timeval timeout, t1;
	//set of file descriptor
	char buffer[5000];
	fd_set readfd;
	/*printf("Canard\n");*/
	//Create a graph for test
	graph__add_node(platypus->graph, "P1");
	/*printf("Canard\n");*/
	graph__add_node(platypus->graph, "P2");
	/*graph__add_link(platypus->graph, "P1", "P2", UP);*/
	/*graph__add_link(platypus->graph, "P1", "P2", DOWN);*/
	//Init the timeout
	timeout.tv_sec = platypus->update_interval;
	timeout.tv_usec = 0;
	gettimeofday(&t1, NULL);
	printf("Timeout %d\nInterval %d seconde(s)\n", platypus->display_timeout, platypus->update_interval);
	printf("Server ready\n");
	while(1)
	{
		FD_ZERO(&readfd);
		int max_fd = platypus__fill_fd_set(platypus, &readfd);
		if(select(max_fd + 1, &readfd, NULL, NULL, &timeout) < 0)
			perror("select");
		if(is_it_timeout(&timeout))
		{
			timeout.tv_sec = platypus->update_interval;
			timeout.tv_usec = 0;
			platypus->time_before_interval = 0;
			platypus__periodic_work(platypus);
			gettimeofday(&t1, NULL);
		}
		else
		{
			how_many_second_since_last_time(platypus, &t1, &timeout);
			platypus->time_before_interval = timeout.tv_sec + (timeout.tv_sec > 0 && timeout.tv_usec > 500000 ? 1 : 0);
			if(platypus->time_before_interval < 0)
				platypus->time_before_interval = 0;
		}
		platypus__check_event(platypus, &readfd);
	}
	close(platypus->socket_server);
	return 0;
}

void how_many_second_since_last_time(struct Platypus* platypus, struct timeval * t1, struct timeval * real_timeout)
{
	struct timeval t2;
	gettimeofday(&t2, NULL);
	real_timeout->tv_sec = platypus->update_interval - (t2.tv_sec-t1->tv_sec);
	real_timeout->tv_usec = 1000000 - (t2.tv_usec-t1->tv_usec);
}
