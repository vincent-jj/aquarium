#pragma once

#include <stdbool.h>

#include "list.h"
#include "err_exit.h"
#include "couple.h"




// Sides of a monitor
typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT,
	ERROR
} Direction;

typedef struct graph_node
{
    char* name;
    // Adjecent nodes: Up, Down, Left, Right
    struct graph_node* neighbors[4];
    //fish that are stopped or not started
    List* all_stopped_fish;
    //fish that the client doesn't manage yet (mainly because they are too recent)
    List* all_fish;
    //fish that the client manage must contain some fish_info
    List* all_send_fish;
    //Fish that just arrived from an other node
    List* recently_arrived_fish;
    bool has_client;
} Graph_node;

typedef struct graph
{
    List* nodes; //Should contain (struct Node*)
} Graph;



Graph_node* graph__create_node(char* name);
void graph__destroy_node(Graph_node* node);
void graph__set_node_neighbor(Graph_node* node1, Direction n1Side, Graph_node* node2);
Graph_node* graph__node_up_to(Graph_node* node);
Graph_node* graph__node_down_to(Graph_node* node);
Graph_node* graph__node_left_to(Graph_node* node);
Graph_node* graph__node_right_to(Graph_node* node);
Graph_node* graph__node_next_to(Graph_node* node, Direction dir);
Direction graph__opposite_direction(Direction dir);
bool graph__is_node_connected(Graph_node* node);
bool graph__has_node_connected(Graph_node* node, Direction dir);
void graph__display_node(Graph_node* node);
Direction graph__most_logical_direction_link(Couple pos1, Couple pos2);


Graph* graph__create(void);
void graph__destroy(Graph* graph);
Graph_node* graph__add_node(Graph* graph, char* name);
void graph__del_node(Graph* graph, char* name);
void graph__add_link(Graph* graph, char* n1, char* n2, Direction n1Side);
void graph__del_link(Graph* graph, char* n1, char* n2, Direction n1Side);
Direction graph__try_add_link(Graph* graph, char* n1, char* n2);
Graph_node* graph__get_node(Graph* graph, char* name);
bool graph__is_empty(Graph* graph);
void graph__display(Graph* graph);
void graph__load(Graph* graph, char* filename);
void graph__save(Graph* graph, char* filename);
void graph__clear(Graph* graph);
bool graph__node_exist(Graph* graph, char* name);
#define graph__for_each_node(graph, node) list__for_each(graph->nodes, node)
