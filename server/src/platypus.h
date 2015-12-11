#pragma once
#include <sys/types.h>
#include <stdbool.h>
#include "fish.h"
#include "graph.h"

#define CONFIG_FILE "controller.cfg"
#define HELLO_IN_AS "hello in as "
#define HELLO "hello"
#define NO_GREETING "no greeting\n"
#define GREETING "greeting "
#define GET_FISHES "getFishes"
#define GET_FISHES_CONTINUOUSLY "getFishesContinuously"
#define PING "ping "
#define PONG "pong "
#define LOG_OUT "log out"
#define BYE "bye\n"
#define OK_MSG "OK\n"
#define NOK_MSG "NOK\n"
#define START_FISH "startFish "
#define DEL_FISH "delFish "
#define ADD_FISH "addFish "
#define REMOVE "remove "
#define LOAD "load "
#define SAVE "save "
#define SHOW "show"
#define ADD_LINK "add link "
#define DEL_LINK "del link "
#define CONFIG_PORT "controller-port"
#define CONFIG_DISPLAY_TIMEOUT "display-timeout-value"
#define CONFIG_INTERVAL "fish-update-interval"

struct Client
{
	Graph_node* node_taken;
	int socket;
	bool asked_for_notify;
	//last_contact is in seconds
	int last_contact;
};
typedef struct Client Client;

struct Platypus
{
	int socket_server;
	//must contain struct Client
	List* all_client;
	//Must contain int for all sockets which are note use for doing aquarium (no node for them)
	List* all_connected;
	//must contain char* with all names
	List* mobility_pattern;
	Graph* graph;
	unsigned short port;
	short display_timeout;
	short update_interval;
	short time_before_interval;
};

Client* platyupus__get_client_from_fd(struct Platypus* platypus, int fd);
struct Platypus* platypus__create(void);
void platypus__clear(struct Platypus* platypus);
void platypus__destroy(struct Platypus* platypus);
void platypus__periodic_work(struct Platypus* platypus);
void platypus__check_event(struct Platypus* platypus, fd_set* readfd);
void platypus__check_event_aquarium(struct Platypus* platypus, fd_set* readfd);
void platypus__check_event_connected(struct Platypus* platypus, fd_set* readfd);
void platypus__check_event_other(struct Platypus* platypus, fd_set* readfd);
bool platypus__connected_execute_commande(struct Platypus* platypus, int fd, char* data);
/*
 * Execute a command for a client (get fish, add fish, log out)
 * DO NOT use this function out of platypus__check_event_aquarium !! (because of log out)
 */
bool platypus__client_execute_commande(struct Platypus* platypus, Client* client, char* data);
int platypus__fill_fd_set(struct Platypus* platypus, fd_set* readfd);
bool platypus__does_name_for_fish_exist(struct Platypus* platypus, char* name);
bool platypus__add_fish(struct Platypus* platypus, char* fish_name, char* pattern_name, int x, int y, int width, int height, Graph_node* node);
bool platypus__start_fish(struct Platypus* platypus, char* fish_name);
bool platypus__del_fish(struct Platypus* platypus, char* fish_name);
void platypus__load_config_file(struct Platypus* platypus, char* filename);
void platypus__show(struct Platypus* platypus);
Client* platypus__get_client_for_node_name(struct Platypus* platypus, char* name);
Graph_node* platypus__get_free_client_node(struct Platypus* platypus);
Fish* platypus__get_fish(struct Platypus* platypus, char* name);
Graph_node* platypus__get_node_of_fish(struct Platypus* platypus, char* name);
void platypus__compute_goal_for_fish(struct Platypus* platypus, Fish* fish);
Client* client__create(Graph_node* node, int socket);
void client__destroy(Client* client);
Graph_node* platypus__get_node_by_name(struct Platypus* platypus, char* name);
void platypus__parse_line_config_file(struct Platypus* platypus, char* line);
