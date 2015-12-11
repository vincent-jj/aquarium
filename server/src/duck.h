#pragma once
#include <sys/types.h>
#include <stdbool.h>
#include "fish.h"
#include "graph.h"

struct Client;
typedef struct Client Client;
#define LIST_NOTIFY "list"

void notify_client(Client* client);
void make_fish_move(Fish* fish);
char* get_data_from_socket(int socket, bool* is_socket_closed);
char* get_data_from_fd(int socket, bool* is_socket_closed, bool is_socket);
char* get_data_from_stdin(int socket);
char* split_into(char* line, char separator, List* result);
bool is_it_timeout(struct timeval* timeout);
bool parse_argument_to_add_fish(char* line, char** fish_name, char** pattern_name, int* x, int* y, int* width, int* height);
char* strconcat(char* str1, char* str2);
bool fish_is_near_border(Fish* fish);
Direction get_border_of_fish(Fish* fish);
void trim_end(char* str);

/*
 * Put data of client in order before destroy it
 * The function does destroy the client
 */
void prepare_remove_of_client(Client* client);
/*
 * Add the fish on the new node given his position, and return true if the fish has moved
 * However, it doesn't remove the fish from the current_node. (essentiely because we can't know if we are on for each or not)
 */
bool make_fish_change_node(Fish* fish, Graph_node* current_node);

