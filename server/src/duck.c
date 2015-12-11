#include "duck.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include "list.h"
#include "position.h"
#include "graph.h"
#include "platypus.h"

bool is_it_timeout(struct timeval* timeout)
{
	return (timeout->tv_sec < 0 || (timeout->tv_sec == 0 && timeout->tv_usec < 0) || (timeout->tv_sec == 0 && timeout->tv_usec == 0));
}
char* get_data_from_socket(int socket, bool* is_socket_closed)
{
	return get_data_from_fd(socket, is_socket_closed, true);
}
char* get_data_from_stdin(int socket)
{
	return get_data_from_fd(socket, NULL, false);
}
char* get_data_from_fd(int socket, bool* is_socket_closed, bool is_socket)
{
	char *data = NULL;
	int received_byte = 0;
	int read_byte = 0;
	if(is_socket_closed != NULL)
		*is_socket_closed = false;
	//On calcule combien il y a d'octet à lire
	ioctl(socket, FIONREAD, &received_byte);

	//Deconnexion du client
	if(received_byte == 0)
	{
		//On ferme la socket
		close(socket);
		if(is_socket_closed != NULL)
		{
			printf("Deconnection\n");
			*is_socket_closed = true;
		}
		return NULL;
	}
	//On alloue en conséquence
	data = malloc(sizeof(char)*(received_byte+1));
	//On lit les données
	if(is_socket)
		read_byte = recv(socket, data, received_byte, 0);
	else
		read_byte = read(socket, data, received_byte);

	if( read_byte < 0)
	{
		perror("recv");
		free(data);
		return NULL;
	}
	data[read_byte] = 0;
	if(read_byte != received_byte) //weird
		printf("What the fuck !?\n");//No buffer overflow here
	/*
	 * Let's think about the previous if (even if it's weird)
	 * read_byte can't be more than received_byte, because of read or recv size
	 * So if they are different, it's because read_byte is less than received_byte, why would that happend ?
	 */
	return data;
}
void trim_end(char* str)
{
	int size = strlen(str);
	while(size > 0 && str[size-1] < 32)
	{
		str[size-1] = 0;
		--size;
	}
}
char* split_into(char* line, char separator, List* result)
{
	//Each line in result must be allocated from a different malloc
	//Each line end with the null-byte
	//the separator had been removed before
	int previous_idx = 0;
	int idx = 0;
	while(line[idx] != 0)
	{
		if(line[idx] == separator)
		{
			//allocate a new string
			char* tmp = calloc(sizeof(char), (idx - previous_idx + 1));
			if(tmp == NULL)
				(perror("malloc"), exit(-1));
			//Copy into it
			memcpy(tmp, line + previous_idx, (idx - previous_idx));
			//Remove the separator and replace it by a null-byte
			tmp[idx - previous_idx] = 0;
			trim_end(tmp);
			list__add_end(result, tmp);
			previous_idx = idx + 1;
		}
		++idx;
	}
	return line + previous_idx;//return the remaining line
}
bool parse_argument_to_add_fish(char* line, char** fish_name, char** pattern_name, int* x, int* y, int* width, int* height)
{
	*fish_name = calloc(strlen(line) + 1, 1);
	*pattern_name = calloc(strlen(line) + 1, 1);
	int nb = sscanf(line, "%s at %dx%d, %dx%d, %s", *fish_name, x, y, width, height, *pattern_name);
	if(nb == 6)
		return true;
	return false;
}
void notify_client(Client* client)
{
	char* line = NULL, *fish_line = NULL;
	Fish* fish = NULL, *other = NULL;
	list__for_each(client->node_taken->all_fish, fish)
	{
		printf("%s\n", fish__get_name(fish));
		Position* p = fish__get_position(fish), *g = fish__get_goal(fish);
		//Create a line for the fish
		int size = asprintf(&fish_line, " [%s at %dx%d,%dx%d,%d]", fish__get_name(fish), p->x, p->y, g->x, g->y, fish__get_remaining_time_on_client(fish));
		//Concatenate the previous fish with the new one
		char* tmp = strconcat(line, fish_line);
		free(line);
		free(fish_line);
		line = tmp;
		list__remove_on_for_each(client->node_taken->all_fish);
		list__add_front(client->node_taken->all_send_fish, fish);
	}
	//
	//Concatenate the command string with the list of fish if any
	if(line != NULL)
	{
		asprintf(&fish_line, "%s%s\n", LIST_NOTIFY, line);
		free(line);
	}
	else
		asprintf(&fish_line, "%s \n", LIST_NOTIFY);
	line = fish_line;
	//Send the data
	printf("Notify \"%s\"\n", line);
	if(send(client->socket, line, strlen(line), 0) < strlen(line))
		perror("send on notify");
	//Then free
	free(line);
}
void make_fish_move(Fish* fish)
{
	int dx = rand()%4, dy = rand()%4;
	Position* p = fish__get_position(fish);
	p->x += dx;
	p->y += dy;
	if(p->x < 0)
		p->x = 0;
	if(p->x > 95)
		p->x = 95;
	if(p->y < 0)
		p->y = 0;
	if(p->y > 95)
		p->y = 95;
}
bool make_fish_change_node(Fish* fish, Graph_node* current_node)
{
	//Doesn't remove the fish from his current_node
	//return true if fish has been changed from his node
	if(fish_is_near_border(fish))
	{
		bool change = rand()%2;
		Graph_node* next_node = NULL;
		if(change)
		{
			Direction tmp = get_border_of_fish(fish);
			Position* pos = fish__get_position(fish);
			switch(tmp)
			{
				case UP:
					next_node = graph__node_up_to(current_node);
				break;
				case DOWN:
					next_node = graph__node_down_to(current_node);
				break;
				case RIGHT:
					next_node = graph__node_right_to(current_node);
				break;
				case LEFT:
					next_node = graph__node_left_to(current_node);
				break;
			}
			if(next_node != NULL)
			{
				switch(tmp)
				{
					case UP:
					case DOWN:
						pos->y = 100 - pos->y;
						break;
					case RIGHT:
					case LEFT:
						pos->x = 100 - pos->x;
						break;
				}
				printf("Move %s from %s to %s\n", fish__get_name(fish), current_node->name, next_node->name);
				list__add_front(next_node->recently_arrived_fish, fish);
				return true;
			}	
		}
	}
	return false;
}
void prepare_remove_of_client(Client* client)
{
	Fish* current_fish = NULL;
	//take care of they fishes
	list__for_each(client->node_taken->all_send_fish, current_fish)
	{
		//Update their position like they would have finish their movement and put the remaining time at 0
		fish__set_position(current_fish, position__create_copy(fish__get_goal(current_fish)));
		fish__set_goal(current_fish, NULL);
		fish__set_remaining_time_on_client(current_fish, 0);
	}
	//Put all send fish into all fish, because the client doesn't have them anymore
	list__append(client->node_taken->all_fish, client->node_taken->all_send_fish);
	list__clear(client->node_taken->all_send_fish);
	if(client->node_taken != NULL)
		client->node_taken->has_client = false;

	client__destroy(client);
}
char* strconcat(char* str1, char* str2)
{
	int size1 = 0, size2 = 0;
	if(str1 != NULL)
		size1 = strlen(str1);
	if(str2 != NULL)
		size2 = strlen(str2);
	char *result = calloc(size1+size2+1, 1);//+1 for the zero-terminator
	/*in real code you would check for errors in malloc here*/
	if(str1 != NULL)
		strcpy(result, str1);
	if(str2 != NULL)
		strcat(result, str2);
	return result;
}
bool fish_is_near_border(Fish* fish)
{
	Position* p = fish__get_position(fish);
	return (p->x < 8 || p->x > 92 || p->y < 8 || p->y > 92);
}
Direction get_border_of_fish(Fish* fish)
{
	Position* p = fish__get_position(fish);
	if(p->y < 8)
		return UP;
	if(p->y > 92)
		return DOWN;
	if(p->x < 8)
		return LEFT;
	if(p->x > 92)
		return RIGHT;
	return -1;
}
