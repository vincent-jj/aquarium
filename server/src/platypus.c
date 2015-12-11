#include "platypus.h"
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include "list.h"
#include "position.h"
#include "graph.h"
#include "duck.h"

struct Platypus* platypus__create(void)
{
	struct Platypus* ret = malloc(sizeof(struct Platypus));
	if(ret == NULL)
		return NULL;
	ret->graph = graph__create();
	//Create the lists
	ret->all_client = list__create();
	ret->all_connected = list__create();
	ret->mobility_pattern = list__create();

	//Load the config file
	platypus__load_config_file(ret, CONFIG_FILE);
	return ret;
}
void platypus__destroy(struct Platypus* platypus)
{
	char* pattern = NULL;
	int* connected = NULL;
	Client* client = NULL;
	//Remove and free all pattern
	list__for_each(platypus->mobility_pattern, pattern)
	{
		free(pattern);
	}
	//Remove all non-client
	list__for_each(platypus->all_connected, connected)
	{
		close(*connected);
		free(connected);
	}
	//Destroy all client
	list__for_each(platypus->all_client, client)
	{
		client__destroy(client);
	}
	list__destroy(platypus->all_client);
	list__destroy(platypus->mobility_pattern);
	list__destroy(platypus->all_connected);
	close(platypus->socket_server);
	graph__destroy(platypus->graph);
	free(platypus);
}
void platypus__periodic_work(struct Platypus* platypus)
{
	Client* client;
	Fish* current_fish = NULL;
	//increase last_contact of each client
	list__for_each(platypus->all_client, client)
	{
		client->last_contact += platypus->update_interval;
		//There is a timeout
		if(client->last_contact > platypus->display_timeout)
		{
			list__remove_on_for_each(platypus->all_client);
			prepare_remove_of_client(client);
		}
	}
	Graph_node* current_node = NULL;
	graph__for_each_node(platypus->graph, current_node)
	{
		//put each send_fish that is finish into all_fish
		list__for_each(current_node->all_send_fish, current_fish)
		{
			fish__decrease_remaining_time_on_client(current_fish, platypus->update_interval);
			if(fish__get_remaining_time_on_client(current_fish) <= 0)
			{
				/*printf("Get back %s on node %s\n", fish__get_name(current_fish), current_node->name);*/
				//Put them back on the server, because the client doesn't have them anymore
				list__remove_on_for_each(current_node->all_send_fish);
				list__add_front(current_node->all_fish, current_fish);
				//Put the fish at the goal, because he moved
				fish__set_position(current_fish, position__create_copy(fish__get_goal(current_fish)));
				//No more goal for the fish
				fish__set_goal(current_fish, NULL);
			}
		}
		//For each fish not on client
		//	-> Change node given their position
		list__for_each(current_node->all_fish, current_fish)
		{
			if(make_fish_change_node(current_fish, current_node))
				list__remove_on_for_each(current_node->all_fish);
		}
	}
	//Can't do this in the same loop because we can try to change the node of fish that just arrived from another node
	//It would be weird -> so two loops is easier
	graph__for_each_node(platypus->graph, current_node)
	{
		//Put recently arrived fish into all_fish so we can make them move
		list__append(current_node->all_fish, current_node->recently_arrived_fish);
		list__clear(current_node->recently_arrived_fish);
		/*printf("Fish move %s\n", current_node->name);*/
		//If the node isn't manage by a client the server move the fishes
		if(!current_node->has_client)
		{
			list__for_each(current_node->all_fish, current_fish)
			{
				make_fish_move(current_fish);
			}
		}
	}
	Client* current_client = NULL;
	//For each client that asked for notify
	list__for_each(platypus->all_client, current_client)
	{
		//compute goal of each fish the client doesn't manage
		list__for_each(current_client->node_taken->all_fish, current_fish)
		{
			platypus__compute_goal_for_fish(platypus, current_fish);
		}
		if(current_client->asked_for_notify)
			notify_client(current_client);//So notify must send goals of fishes
	}
	/*
	 * What happen for the client that are not notify automatically and that doesn't asked for notify ?
	 * Because they are nerver notify -> no fish will be in all_send_fish
	 * So the goal of each fish in all_fish will be computed again at each periodic_work
	 * FIXME But they'll never move !!
	 */
}
int platypus__fill_fd_set(struct Platypus* platypus, fd_set* readfd)
{
	FD_SET(platypus->socket_server, readfd);
	FD_SET(0, readfd);
	int max_fd = platypus->socket_server;
	Client* client = NULL;
	int* connected = NULL;
	//Add all the connected but not client yet
	list__for_each(platypus->all_connected, connected)
	{
		if(*connected > max_fd)
			max_fd = *connected;
		FD_SET(*connected, readfd);
	}
	//Add all the client
	list__for_each(platypus->all_client, client)
	{
		if(client->socket > max_fd)
			max_fd = client->socket;
		FD_SET(client->socket, readfd);
	}
	return max_fd;
}
void platypus__check_event(struct Platypus* platypus, fd_set* readfd)
{
	platypus__check_event_other(platypus, readfd);
	platypus__check_event_aquarium(platypus, readfd);
	platypus__check_event_connected(platypus, readfd);
}
void platypus__check_event_aquarium(struct Platypus* platypus, fd_set* readfd)
{
	List* split_list = list__create();
	Client* current_client = NULL;
	list__for_each(platypus->all_client, current_client)
	{
		if(FD_ISSET(current_client->socket, readfd))
		{
			FD_CLR(current_client->socket, readfd);//it's useful for all_connected, but i'm not sure it is for clients
			bool is_closed;
			char* data = get_data_from_socket(current_client->socket, &is_closed);
			if(is_closed)
			{
				//Put data in a correct state and remove the client
				list__remove_on_for_each(platypus->all_client);
				prepare_remove_of_client(current_client);
				continue;
			}
			//TODO add the previous remaining to data
			char* remaining = split_into(data, '\n', split_list);
			// do something with the remaining
			char* each_line = NULL;
			bool stop_executing = false;
			list__for_each(split_list, each_line)
			{
				//Once log out, don't look any command from this client
				//Just free each previous command
				if(stop_executing || !platypus__client_execute_commande(platypus, current_client, each_line))
					stop_executing = true;
				free(each_line);
			}
			list__clear(split_list);
			free(data);
		}
	}
	list__destroy(split_list);
}
bool platypus__client_execute_commande(struct Platypus* platypus, Client* client, char* data)
{
	//Update last contact in client
	printf("Command : \"%s\"\n", data);
	char* tmp_line = NULL;
	client->last_contact = 0;
	if(strncmp(data, GET_FISHES_CONTINUOUSLY, strlen(GET_FISHES_CONTINUOUSLY)) == 0)
	{
		printf("GET CONTINUE\n");
		client->asked_for_notify = true;
		return true; //-> Keep iterating command for this client
	}
	if(strncmp(data, GET_FISHES, strlen(GET_FISHES)) == 0)
	{
		printf("GET FISH\n");
		Fish* fish = NULL;
		list__for_each(client->node_taken->all_fish, fish)
		{
			Position* goal = fish__get_goal(fish);
			if(goal == NULL)
				platypus__compute_goal_for_fish(platypus, fish);
		}
		//Send back list of all fish
		notify_client(client);
		return true; //-> Keep iterating command for this client
	}
	if(strncmp(data, PING, strlen(PING)) == 0)
	{
		strncpy(data, PONG, strlen(PONG));//Normally doesn't place the null byte from PONG
		asprintf(&tmp_line, "%s\n", data);
		//Send back same string with pong instead
		int sent = send(client->socket, tmp_line, strlen(tmp_line), 0);
		free(tmp_line);
		return true; //-> Keep iterating command for this client
	}
	//Deconnection of a client
	if(strncmp(data, LOG_OUT, strlen(LOG_OUT)) == 0)
	{
		printf("LOGOUT\n");
		send(client->socket, BYE, strlen(BYE), 0);
		//remove current client from list (that's why platypus__client_execute_commande should be use only in platypus__check_event_aquarium)
		list__remove_on_for_each(platypus->all_client);
		prepare_remove_of_client(client);
		return false; //-> stop executing command from this client
	}
	//Order to add fish
	if(strncmp(data, ADD_FISH, strlen(ADD_FISH)) == 0)
	{
		printf("ADD_FISH\n");
		char* fish_name = NULL, *pattern_name = NULL;
		int x, y, width, height;
		if(parse_argument_to_add_fish(data + strlen(ADD_FISH), &fish_name, &pattern_name, &x, &y, &width, &height))
			if(platypus__add_fish(platypus, fish_name, pattern_name, x, y, width, height, client->node_taken))
				send(client->socket, OK_MSG, strlen(OK_MSG), 0);
			else
				send(client->socket, NOK_MSG, strlen(NOK_MSG), 0);
		else
			send(client->socket, NOK_MSG, strlen(NOK_MSG), 0);
		return true; //-> Keep iterating command for this client
	}
	//Order to delete a fish
	if(strncmp(data, DEL_FISH, strlen(DEL_FISH)) == 0)
	{
		printf("DEL_FISH\n");
		//Send back list of all fish
		if(platypus__del_fish(platypus, data + strlen(DEL_FISH)))
			send(client->socket, OK_MSG, strlen(OK_MSG), 0);
		else
			send(client->socket, NOK_MSG, strlen(NOK_MSG), 0);
		return true; //-> Keep iterating command for this client
	}
	//Order to start a fish
	if(strncmp(data, START_FISH, strlen(START_FISH)) == 0)
	{
		printf("START_FISH\n");
		if(platypus__start_fish(platypus, data + strlen(START_FISH)))
			send(client->socket, OK_MSG, strlen(OK_MSG), 0);
		else
			send(client->socket, NOK_MSG, strlen(NOK_MSG), 0);
		return true; //-> Keep iterating command for this client
	}
	return true;
}
void platypus__check_event_connected(struct Platypus* platypus, fd_set* readfd)
{
	int* connected = NULL;
	List* split_list = list__create();
	list__for_each(platypus->all_connected, connected)
	{
		if(FD_ISSET(*connected, readfd))
		{
			FD_CLR(*connected, readfd);//So even if he go in client list, he won't be check after that
			bool is_closed;
			char* data = get_data_from_socket(*connected, &is_closed);
			if(is_closed)
			{
				//client deco -> delete it from all_connected
				list__remove_on_for_each(platypus->all_connected);
				close(*connected);
				continue;
			}
			//TODO add the previous remaining to data
			char* remaining = split_into(data, '\n', split_list);
			// do something with the remaining
			char* each_line = NULL;
			bool stop_executing = false;
			list__for_each(split_list, each_line)
			{
				//Once greeting, we don't have to trade with him, he became a client
				//Just free each previous command
				if(stop_executing || !platypus__connected_execute_commande(platypus, *connected, each_line))
				{
					stop_executing = true;
					//He already been had into the client list
					list__remove_on_for_each(platypus->all_connected);
				}
				free(each_line);
			}
			list__clear(split_list);
			free(data);
		}
	}
	list__destroy(split_list);
}
bool platypus__connected_execute_commande(struct Platypus* platypus, int fd, char* data)
{
	//Do a greeting
	//If no more free node : nogreeting
	//If no arg, get a free node
	// ->Give it node
	//Else
	// ->If node wanted exist and is free
	// 	-> Give it node
	// ->Else
	// 	-> nogreeting
	char* tmp_line = NULL;
	printf("Command (connected) : %s\n", data);
	if(strncmp(data, PING, strlen(PING)) == 0)
	{
		strncpy(data, PONG, strlen(PONG));//Normally doesn't place the null byte from PONG
		asprintf(&tmp_line, "%s\n", data);
		//Send back same string with pong instead
		send(fd, tmp_line, strlen(tmp_line), 0);
		free(tmp_line);
		return true; //-> Keep iterating command for this client
	}
	Graph_node* tmp_node = NULL;
	if(strncmp(data, HELLO_IN_AS, strlen(HELLO_IN_AS)) == 0)
	{
		//Try to find if there is a client on that node
		Client* tmp = platypus__get_client_for_node_name(platypus, data + strlen(HELLO_IN_AS));
		printf("Asking for node %s\n", data + strlen(HELLO_IN_AS));
		if(tmp == NULL)
		{
			//If no node, the result will probably be NULL
			tmp_node = platypus__get_node_by_name(platypus, data + strlen(HELLO_IN_AS));
		}
	}
	if(strncmp(data, HELLO, strlen(HELLO)) == 0)
	{
		if(tmp_node == NULL)
			tmp_node = platypus__get_free_client_node(platypus);
		if(tmp_node == NULL)
		{
			send(fd, NO_GREETING, strlen(NO_GREETING), 0);
			return true;//-> keep iterating on that client (No sure is useful because the graph is full ...)
		}
		//add it to the clients
		Client* new_client = client__create(tmp_node, fd);
		list__add_front(platypus->all_client, new_client);
		char* tmp = strconcat(GREETING, tmp_node->name);
		char* tmp2 = strconcat(tmp, "\n");
		send(fd, tmp2, strlen(tmp2), 0);//Send the greeting message
		free(tmp);
		free(tmp2);
		return false;//-> don't iterate anymore
	}
	return true;//No specific change, keep iterating
}
void platypus__check_event_other(struct Platypus* platypus, fd_set* readfd)
{
	if(FD_ISSET(0, readfd))
	{
		char* command = get_data_from_stdin(0);
		if(command)
		{
			//Remove the last \n
			if(command[strlen(command) - 1] == '\n')
				command[strlen(command) - 1] = 0;

			//For stdin, we assume there is only one command that can be type in
			if(strncmp(command, ADD_LINK, strlen(ADD_LINK)) == 0)
			{
				char first[strlen(command)+1];
				char second[strlen(command)+1];
				char third[strlen(command)+1];

				if(sscanf(command + strlen(ADD_LINK), "%s %s %s", first, second, third) == 3)
				{
					//Check if both node exist
					if(graph__node_exist(platypus->graph, second) && graph__node_exist(platypus->graph, first))
					{
						//Add the link
						if(strncmp(third, "UP", 2) == 0)
							graph__add_link(platypus->graph, first, second, UP);
						else if(strncmp(third, "DOWN", 4) == 0)
							graph__add_link(platypus->graph, first, second, DOWN);
						else if(strncmp(third, "LEFT", 4) == 0)
							graph__add_link(platypus->graph, first, second, LEFT);
						else if(strncmp(third, "RIGHT", 4) == 0)
							graph__add_link(platypus->graph, first, second, RIGHT);
						else
							printf("Error during add_link, need direction (UP, DOWN, LEFT, RIGHT)\n");
					}
				}
			}
			else if(strncmp(command, DEL_LINK, strlen(DEL_LINK)) == 0)
			{
				char first[strlen(command)+1];
				char second[strlen(command)+1];
				char third[strlen(command)+1];
				//Split with space, the second argument is the remaining
				if(sscanf(command + strlen(DEL_LINK), "%s %s %s", first, second, third) == 3)
				{
					//Check if both node exist
					if(graph__node_exist(platypus->graph, second) && graph__node_exist(platypus->graph, first))
					{
						//Add the link
						if(strncmp(third, "UP", 2) == 0)
							graph__del_link(platypus->graph, first, second, UP);
						else if(strncmp(third, "DOWN", 4) == 0)
							graph__del_link(platypus->graph, first, second, DOWN);
						else if(strncmp(third, "LEFT", 4) == 0)
							graph__del_link(platypus->graph, first, second, LEFT);
						else if(strncmp(third, "RIGHT", 4) == 0)
							graph__del_link(platypus->graph, first, second, RIGHT);
						else
							printf("Error during del_link, need direction (UP, DOWN, LEFT, RIGHT)\n");
					}
				}
			}
			else if(strncmp(command, REMOVE, strlen(REMOVE)) == 0)
			{
				char* name_of_node = command + strlen(REMOVE);
				Client* client = platypus__get_client_for_node_name(platypus, name_of_node), *cli = NULL;
				if(client != NULL)
				{
					//Put the iterator on client
					list__for_each(platypus->all_client, cli)
					{
						if(cli == client)
							break;
					}
					//Then delete it (The client is somewhere in the list, else we would'nt have found it before
					list__remove(platypus->all_client);
					//Clear and destroy it
					prepare_remove_of_client(client);
				}
			}
			else if(strncmp(command, LOAD, strlen(LOAD)) == 0)
			{
				char* filename = command + strlen(LOAD);
				platypus__clear(platypus);
				//Finally load the new graph
				graph__load(platypus->graph, filename);
			}
			else if(strncmp(command, SAVE, strlen(SAVE)) == 0)
				graph__save(platypus->graph, command + strlen(SAVE));
			else if(strncmp(command, SHOW, strlen(SHOW)) == 0)
				platypus__show(platypus);
		}
	}
	if(FD_ISSET(platypus->socket_server, readfd))
	{
		//Accept the connexion and put it into all_connected
		int *csock = malloc(sizeof(int));
		struct sockaddr_in csin;
		int crecsize = sizeof csin;
		*csock = accept(platypus->socket_server, (struct sockaddr *) &csin, (socklen_t*)&crecsize);
		list__add_front(platypus->all_connected, csock);
		printf("New connection\n");
	}
}
bool platypus__does_name_for_fish_exist(struct Platypus* platypus, char* name)
{
	Graph_node* current_node = NULL;
	Fish* current_fish = NULL;
	graph__for_each_node(platypus->graph, current_node)
	{
		list__for_each(current_node->all_stopped_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return true;
		}
		list__for_each(current_node->all_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return true;
		}
		list__for_each(current_node->all_send_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return true;
		}
	}
	return false;
}
Client* platypus__get_client_for_node_name(struct Platypus* platypus, char* name)
{
	Client* client = NULL;
	list__for_each(platypus->all_client, client)
	{
		if(client->node_taken != NULL && strncmp(name, client->node_taken->name, strlen(name)) == 0)
			return client;
	}
	return NULL;
}
Graph_node* platypus__get_free_client_node(struct Platypus* platypus)
{
	Graph_node* current_node = NULL;
	graph__for_each_node(platypus->graph, current_node)
	{
		if(current_node->has_client == false)
			return current_node;
	}
	return NULL;
}
bool platypus__add_fish(struct Platypus* platypus, char* fish_name, char* pattern_name, int x, int y, int width, int height, Graph_node* node)
{
	//check the name, the location
	if(width <= 0 || height <= 0 || x < 0 - width || x > 100 || y < 0 - height || y > 100)
		return false;
	if(platypus__does_name_for_fish_exist(platypus, fish_name))
		return false;

	Position* p = position__create_position(x, y);
	Position* s = position__create_position(width, height);
	Fish* new_fish = fish__create_fish(fish_name, p, s, pattern_name);
	if(new_fish == NULL)
	{
		position__destroy(p);
		position__destroy(s);
		return false;
	}
	list__add_front(node->all_stopped_fish, new_fish);
	fprintf(stderr, "Add fish %s at %dx%d, %dx%d, %s on node %s\n", fish_name, x, y, width, height, pattern_name, node->name);
	return true;
}
bool platypus__start_fish(struct Platypus* platypus, char* fish_name)
{
	// -> Check the name and if the fish has already been started
	Fish* current_fish = NULL, *fish = platypus__get_fish(platypus, fish_name);
	if(fish == NULL)
		return false;
	Graph_node* node = platypus__get_node_of_fish(platypus, fish_name);
	if(fish__is_started(fish))
		return false;
	list__for_each(node->all_stopped_fish, current_fish)
	{
		if(fish == current_fish)
		{
			fish__set_started(fish, true);
			list__add_front(node->all_fish, fish);
			list__remove(node->all_stopped_fish);
			platypus__compute_goal_for_fish(platypus, fish);
			fprintf(stderr, "Start fish %s on %s\n", fish_name, node->name);
			return true;
		}
	}
	return false;
}
bool platypus__del_fish(struct Platypus* platypus, char* fish_name)
{
	//Don't be afraid of remove it from all_send_fish if it is on the client
	// -> check the name
	/*fish_name[strlen(fish_name)-1] = 0;*/
	printf("Try del fish %s\n", fish_name);
	
	Fish* current_fish = NULL, *fish = platypus__get_fish(platypus, fish_name);
	if(fish == NULL)
	{
		printf("Fish \"%s\" is nowhere to be found\n");
		return false;
	}
	Graph_node* node = platypus__get_node_of_fish(platypus, fish_name);
	if(node == NULL)
	{
		printf("node null\n");
		return false;
	}
	List* list_of_fish = NULL;
	//Get the right list of fish
	if(!fish__is_started(fish))
		list_of_fish = node->all_stopped_fish;
	else
		list_of_fish = node->all_fish;
	bool found = false;
	//Iterate on this list to find the fish
	list__for_each(list_of_fish, current_fish)
	{
		//Then delete it
		if(current_fish == fish)
		{
			list__remove(list_of_fish);
			found = true;
			break;
		}
	}
	if(!found)
	{
		list_of_fish = node->all_send_fish;
		list__for_each(list_of_fish, current_fish)
		{
			//Then delete it
			if(current_fish == fish)
			{
				list__remove(list_of_fish);
				break;
			}
		}
	}
	//He can't be on recently_arrived_fish because when fish arrive in this list they are moved in all_fish and no client command can be call at this moment
	fish__destroy(fish);
	fprintf(stderr, "Del fish %s\n", fish_name);
	return true;
}
void platypus__load_config_file(struct Platypus* platypus, char* filename)
{
	platypus->port = 33400;
	platypus->display_timeout = 45;
	platypus->update_interval = 5;
	FILE* file = fopen(filename, "r");
	if(file == NULL)
		return;
	fprintf(stderr, "Load config file : %s\n", filename);
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	while ((read = getline(&line, &len, file)) != -1)
	{
		//If it begin with a #, it's a comment
		if(line[0] != '#' && line[0] != '\n')	
			platypus__parse_line_config_file(platypus, line);
		free(line);
		//len need to be 0 so getline will allocate
		len = 0;
	}

	fclose(file);
	if (line)
		free(line);
}
Fish* platypus__get_fish(struct Platypus* platypus, char* name)
{
	Graph_node* current_node = NULL;
	Fish* current_fish = NULL;
	graph__for_each_node(platypus->graph, current_node)
	{
		list__for_each(current_node->all_stopped_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_fish;
		}
		list__for_each(current_node->all_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_fish;
		}
		list__for_each(current_node->all_send_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_fish;
		}
	}
	return NULL;
}
Graph_node* platypus__get_node_of_fish(struct Platypus* platypus, char* name)
{
	Graph_node* current_node = NULL;
	Fish* current_fish = NULL;
	graph__for_each_node(platypus->graph, current_node)
	{
		list__for_each(current_node->all_stopped_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_node;
		}
		list__for_each(current_node->all_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_node;
		}
		list__for_each(current_node->all_send_fish, current_fish)
		{
			if(strncmp(name, fish__get_name(current_fish), strlen(name)) == 0)
				return current_node;
		}
	}
	return NULL;
}
void platypus__show(struct Platypus* platypus)
{
	graph__display(platypus->graph);
	Client* tmp = NULL;
	if(list__get_size(platypus->all_client) > 0)
		list__for_each(platypus->all_client, tmp)
			printf("%s have client\n", tmp->node_taken->name);
	else
		printf("No client\n");
}
void platypus__clear(struct Platypus* platypus)
{
	Client* client = NULL;
	//Remove and destoy all client
	list__for_each(platypus->all_client, client)
	{
		prepare_remove_of_client(client);
		client__destroy(client);
	}
	list__clear(platypus->all_client);
	//Clear the graph after client, because prepare_remove_of_client might use node_taken
	graph__clear(platypus->graph);
}
void platypus__compute_goal_for_fish(struct Platypus* platypus, Fish* fish)
{
	
	//create the goal and the time it will need using update_interval, it will be easier
	int border;
	//Get the border of the fish -1 else
	border = get_border_of_fish(fish);

	//Choose the next border
	int next_border = border;
	while(next_border == border)
		next_border = rand()%4;

	//Generate the next_goal so the fish will be near a border
	Position* next_goal = position__create_position(200, 200);
	switch(next_border)
	{
		case 0: //Up
			next_goal->x = rand()%100;
			next_goal->y = rand()%7;
		break;
		case 1: //Down
			next_goal->x = rand()%100;
			next_goal->y = (rand()%7) + 93;
		break;
		case 2: //Left
			next_goal->y = rand()%100;
			next_goal->x = rand()%7;
		break;
		default: //Right
			next_goal->y = rand()%100;
			next_goal->x = (rand()%7) + 93;
		break;
	}
	fish__set_goal(fish, next_goal);
	fish__set_remaining_time_on_client(fish, platypus->update_interval*(1 + rand()%5) + platypus->time_before_interval);
	printf("%s is given for %d sec\n", fish__get_name(fish), fish__get_remaining_time_on_client(fish));
}
Client* client__create(Graph_node* node, int socket)
{
	Client* ret = malloc(sizeof(Client));
	if(ret == NULL)
		return NULL;
	ret->asked_for_notify = false;
	ret->node_taken = node;
	ret->node_taken->has_client = true;
	ret->socket = socket;
	ret->last_contact = 0;
	return ret;
}
void client__destroy(Client* client)
{
	close(client->socket);
	free(client);
}
Graph_node* platypus__get_node_by_name(struct Platypus* platypus, char* name)
{
	Graph_node* node = NULL;
	graph__for_each_node(platypus->graph, node)
		if(strncmp(node->name, name, strlen(node->name)) == 0)
			return node;
	return NULL;
}
void platypus__parse_line_config_file(struct Platypus* platypus, char* line)
{
	if(strncmp(line, CONFIG_PORT, strlen(CONFIG_PORT)) == 0)
	{
		if(sscanf(line, CONFIG_PORT" = %hu", &platypus->port) != 1) 
			printf("Config file expected : "CONFIG_PORT" = <nb>\n");
	}
	else if(strncmp(line, CONFIG_DISPLAY_TIMEOUT, strlen(CONFIG_DISPLAY_TIMEOUT)) == 0)
	{
		if(sscanf(line, CONFIG_DISPLAY_TIMEOUT" = %hd", &platypus->display_timeout) != 1) 
			printf("Config file expected : "CONFIG_DISPLAY_TIMEOUT" = <nb>\n");
	}
	else if(strncmp(line, CONFIG_INTERVAL, strlen(CONFIG_INTERVAL)) == 0)
	{
		if(sscanf(line, CONFIG_INTERVAL" = %hd", &platypus->update_interval) != 1) 
			printf("Config file expected : "CONFIG_INTERVAL" = <nb>\n");
	}
}
