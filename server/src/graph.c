#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#include "graph.h"
#include "dot_parser.h"
#include "couple.h"
#include "err_exit.h"

#define MAX_NODES 100


Graph_node* graph__create_node(char* name)
{
	Graph_node* newNode = malloc(sizeof(Graph_node));
	if(!newNode)
		err_exit("Unable to allocate memory for new node");

	newNode->name = name;
	int i;
	for(i=0; i < 3; i++)
		newNode->neighbors[i] = NULL;
	newNode->all_stopped_fish = list__create();
	newNode->all_fish = list__create();
	newNode->all_send_fish = list__create();
	newNode->recently_arrived_fish = list__create();
	newNode->has_client = false;

	return newNode;
}
void graph__destroy_node(Graph_node* node)
{
	list__destroy(node->all_stopped_fish);
	list__destroy(node->all_fish);
	list__destroy(node->all_send_fish);
	list__destroy(node->recently_arrived_fish);
	free(node);
}
void graph__set_node_neighbor(Graph_node* node1, Direction n1Side, Graph_node* node2)
{
	switch(n1Side)
	{
		case UP:
			node1->neighbors[0] = node2;
			break;
		case DOWN:
			node1->neighbors[1] = node2;
			break;
		case LEFT:
			node1->neighbors[2] = node2;
			break;
		case RIGHT:
			node1->neighbors[3] = node2;
			break;
	}
}
Graph_node* graph__node_up_to(Graph_node* node)
{
	return node->neighbors[0];
}
Graph_node* graph__node_down_to(Graph_node* node)
{
	return node->neighbors[1];
}
Graph_node* graph__node_left_to(Graph_node* node)
{
	return node->neighbors[2];
}
Graph_node* graph__node_right_to(Graph_node* node)
{
	return node->neighbors[3];
}
Graph_node* graph__node_next_to(Graph_node* node, Direction dir)
{
	switch(dir)
	{
		case UP:
			return graph__node_up_to(node);
		case DOWN:
			return graph__node_down_to(node);
		case LEFT:
			return graph__node_left_to(node);
		case RIGHT:
			return graph__node_right_to(node);
	}
}
Direction graph__opposite_direction(Direction dir)
{
	switch(dir)
	{
		case UP:
			return DOWN;
		case DOWN:
			return UP;
		case LEFT:
			return RIGHT;
		case RIGHT:
			return LEFT;
	}
}
bool graph__is_node_connected(Graph_node* node)
{
	return (graph__has_node_connected(node, UP)||
			graph__has_node_connected(node, DOWN)||
			graph__has_node_connected(node, LEFT)||
			graph__has_node_connected(node, RIGHT));
}
bool graph__has_node_connected(Graph_node* node, Direction dir)
{
	switch (dir)
	{
		case UP:
			if(graph__node_up_to(node) != NULL)
				return true;
			break;
		case DOWN:
			if(graph__node_down_to(node) != NULL)
				return true;
			break;
		case LEFT:
			if(graph__node_left_to(node) != NULL)
				return true;
			break;
		case RIGHT:
			if(graph__node_right_to(node) != NULL)
				return true;
			break;
		default:
			err_exit("Direction does not exist");
	}
	return false;
}
void graph__display_node(Graph_node* node)
{
	if(!graph__is_node_connected(node))
		printf("%s (non-connected node)", node->name);
	else
	{
		printf("%s: ", node->name);

		printf("UP:");
		if(graph__has_node_connected(node, UP))
			printf("%s	", graph__node_up_to(node)->name);
		else
			printf("na ");

		printf("DOWN:");
		if(graph__has_node_connected(node, DOWN))
			printf("%s	", graph__node_down_to(node)->name);
		else
			printf("na ");

		printf("LEFT:");
		if(graph__has_node_connected(node, LEFT))
			printf("%s	", graph__node_left_to(node)->name);
		else
			printf("na ");

		printf("RIGHT:");
		if(graph__has_node_connected(node, RIGHT))
			printf("%s	", graph__node_right_to(node)->name);
		else
			printf("na ");
	}
	printf("\n");
}
Direction graph__most_logical_direction_link(Couple pos1, Couple pos2)
{
	int x1 = pos1.x;
	int y1 = pos1.y;
	int x2 = pos2.x;
	int y2 = pos2.y;
	int y21 = y2 - y1;
	int x21 = x2 - x1;

	if(y21 < 0) // Node1 over node2
	{
		if(abs(x21) < abs(y21)) // More on the top than on the sides
			return DOWN;
		else
		{
			if(x21 > 0)
				return RIGHT;
			else {
				return LEFT;
			}
		}
	}
	else
	{
		if(abs(x21) < abs(y21)) // More under than on the sides
			return UP;
		else
		{
			if(x21 > 0)
				return RIGHT;
			else {
				return LEFT;
			}
		}
	}

}

Graph* graph__create(void)
{
	Graph* graph = malloc(sizeof(Graph));
	if(!graph)
		err_exit("Unable to allocate memory for graph");
	graph->nodes = list__create();

	return graph;
}
void graph__destroy(Graph* graph)
{
	if(graph && list__get_size(graph->nodes) > 0)
	{
		Graph_node* currentNode = NULL;
		graph__for_each_node(graph, currentNode)
		{
			graph__destroy_node(currentNode);
			list__remove_on_for_each(graph->nodes);
		}
		free(graph);
	}
}
Graph_node* graph__add_node(Graph* graph, char* name)
{
	Graph_node* node = graph__create_node(name);
	list__add_front(graph->nodes, (void*)node);
	return node;
}
void graph__del_node(Graph* graph, char* name)
{
	if(!graph)
		err_exit("Graph not valid");

	Graph_node* currentNode = NULL;
	if(!graph__is_empty(graph))
	{
		graph__for_each_node(graph, currentNode)
		{
			if(!strcmp(currentNode->name, name))
			{
				graph__destroy_node(currentNode);
				list__remove_on_for_each(graph->nodes);
			}
		}
	}
	else
		err_exit("Trying to access node of an empty graph");
}
void graph__add_link(Graph* graph, char* n1, char* n2, Direction n1Side)
{
	Graph_node* currentNode = NULL;
	if(!graph__is_empty(graph))
	{
		Graph_node* node1 = NULL;
		Graph_node* node2 = NULL;
		graph__for_each_node(graph, currentNode)
		{
			if(!strcmp(currentNode->name, n1))
				node1 = currentNode;
			if(!strcmp(currentNode->name, n2))
				node2 = currentNode;
		}
		if(node1 == NULL || node2 == NULL)
			err_exit("Graph node not found");

		Direction n2Side = graph__opposite_direction(n1Side);
		graph__set_node_neighbor(node1, n1Side, node2);
		graph__set_node_neighbor(node2, n2Side, node1);

	}
}
Direction graph__try_add_link(Graph* graph, char* n1, char* n2)
{
	return ERROR;
}
void graph__del_link(Graph* graph, char* n1, char* n2, Direction n1Side)
{
	Graph_node* currentNode = NULL;
	if(!graph__is_empty(graph))
	{
		Graph_node* node1 = NULL;
		Graph_node* node2 = NULL;
		graph__for_each_node(graph, currentNode)
		{
			if(!strcmp(currentNode->name, n1))
				node1 = currentNode;
			if(!strcmp(currentNode->name, n2))
				node2 = currentNode;
		}
		if(node1 == NULL || node2 == NULL)
			err_exit("Graph node not found");

		Direction n2Side = graph__opposite_direction(n1Side);
		graph__set_node_neighbor(node1, n1Side, NULL);
		graph__set_node_neighbor(node2, n2Side, NULL);

	}
}
Graph_node* graph__get_node(Graph* graph, char* name)
{
	if(!graph)
		err_exit("Graph not valid");

	Graph_node* currentNode = NULL;
	if(!graph__is_empty(graph))
	{
		graph__for_each_node(graph, currentNode)
		{
			if(!strcmp(currentNode->name, name))
				return currentNode;
		}
	}
	else
		err_exit("Trying to access node of an empty graph");
}
bool graph__is_empty(Graph* graph)
{
	return (graph->nodes->head == NULL);
}
void graph__display(Graph* graph)
{
	if(!graph)
		err_exit("Graph not valid");

	Graph_node* currentNode = NULL;
	if(!graph__is_empty(graph))
		graph__for_each_node(graph, currentNode)
		{
			graph__display_node(currentNode);
		}
	else
		printf("Empty graph, no vertices");
}
void graph__load(Graph* graph, char* filename)
{
	FILE * fp;
	char aquariumName[MAX_WORD_LENGTH];
	int nodeOrLink = -1;

	// Je sais, c'est sale et j'aurais du utiliser une hashmap, mais j'ai trop la flemme là
	Couple nodesPos[MAX_NODES];
	char nodesNames[MAX_NODES][MAX_WORD_LENGTH];
	char linkLabels[MAX_NODES * 4][2][MAX_WORD_LENGTH];
	int nodesCount = 0;
	int linksCount = 0;

	fp = fopen(filename, "r");
	if (fp == NULL)
		err_exit("File can not be opened");


	dot_parser__f_get_aquarium_name(fp, aquariumName);
	do
	{
		char nodes[2][MAX_WORD_LENGTH];
		nodeOrLink = dot_parser__f_is_node_or_link(fp, nodes);

		if(nodeOrLink == 0)
		{
			char label[MAX_WORD_LENGTH];
			Couple pos;
			dot_parser__f_get_node(fp, label, &pos);
			nodesPos[nodesCount] = pos;
			strcpy(nodesNames[nodesCount], nodes[0]);
			nodesCount++;
		}
		else if(nodeOrLink == 1)
		{
			char label[MAX_WORD_LENGTH];
			Couple pos;
			dot_parser__f_get_link(fp, label);
			strcpy(linkLabels[linksCount][0], nodes[0]);
			strcpy(linkLabels[linksCount][1], nodes[1]);
			linksCount++;
		}
	} while(nodeOrLink != -1);

	fclose(fp);


	int i;
	for(i=0; i < nodesCount; i++)
		graph__add_node(graph, nodesNames[i]);

	for(i=0; i < linksCount; i++)
	{
		int firstNodeindex = -1;
		int secondNodeIndex = -1;
		int j;

		for(j=0; j < nodesCount; j++)
		{
			if(!strcmp(linkLabels[i][0], nodesNames[j]))
				firstNodeindex = j;
			if(!strcmp(linkLabels[i][1], nodesNames[j]))
				secondNodeIndex = j;
		}

		Direction dir = graph__most_logical_direction_link(
			nodesPos[firstNodeindex], nodesPos[secondNodeIndex]);

		graph__add_link(graph, nodesNames[firstNodeindex], nodesNames[secondNodeIndex], dir);
	}

	Graph_node* n;
	char nodesNamesToCheckIfTwiceMothaFucka[MAX_NODES][MAX_WORD_LENGTH];
	int nodesNamesToCheckIfTwiceMothaFuckaIndex = 0;
	graph__for_each_node(graph, n)
	{
		strcpy(nodesNamesToCheckIfTwiceMothaFucka[nodesNamesToCheckIfTwiceMothaFuckaIndex++], n->name);
	}
	int nodeNb = 0;
	graph__for_each_node(graph, n)
	{
		for(nodesNamesToCheckIfTwiceMothaFuckaIndex = 0; nodesNamesToCheckIfTwiceMothaFuckaIndex < nodesCount; nodesNamesToCheckIfTwiceMothaFuckaIndex++)
		{
			if(nodeNb != nodesNamesToCheckIfTwiceMothaFuckaIndex && !strcmp(n->name, nodesNamesToCheckIfTwiceMothaFucka[nodesNamesToCheckIfTwiceMothaFuckaIndex]))
				err_exit("Cannot have two nodes with the same name in a graph");
		}
		nodeNb++;
	}


	/*printf("Finished loading %s\n", aquariumName);*/
}
void graph__save(Graph* graph, char* filename)
{
	/*FILE * fp;*/
	/*char aquariumName[MAX_WORD_LENGTH];*/
	/*int nodeOrLink = -1;*/

	/*// Je sais, c'est sale et j'aurais du utiliser une hashmap, mais j'ai trop la flemme là*/
	/*Couple nodesPos[MAX_NODES];*/
	/*char* nodesNames[MAX_NODES];*/
	/*char* linkLabels[MAX_NODES * 4][2];*/
	/*int nodesCount = 0;*/
	/*int linksCount = 0;*/

	/*fp = fopen(filename, "w");*/
	/*if (fp == NULL)*/
	/*err_exit("File can not be opened");*/


	/*f_getAquariumName(fp, aquariumName);*/
	/*do*/
	/*{*/
	/*char nodes[2][MAX_WORD_LENGTH];*/
	/*nodeOrLink = f_isNodeOrLink(fp, nodes);*/

	/*if(nodeOrLink == 0)*/
	/*{*/
	/*char label[MAX_WORD_LENGTH];*/
	/*Couple pos;*/
	/*f_getNode(fp, label, &pos);*/
	/*nodesPos[nodesCount] = pos;*/
	/*nodesNames[nodesCount] = nodes[0];*/
	/*nodesCount++;*/
	/*[>printf("%s: %s, %d, %d\n", label, nodes[0], pos.x, pos.y);<]*/
	/*}*/
	/*else if(nodeOrLink == 1)*/
	/*{*/
	/*char label[MAX_WORD_LENGTH];*/
	/*Couple pos;*/
	/*f_getNode(fp, label, &pos);*/
	/*linkLabels[linksCount][0] = nodes[0];*/
	/*linkLabels[linksCount][1] = nodes[1];*/
	/*linksCount++;*/
	/*[>printf("%s: %s.%s \n",label, nodes[0], nodes[1]);<]*/
	/*}*/
	/*} while(nodeOrLink != -1);*/

	/*fclose(fp);*/

	/*int i;*/
	/*for(i=0; i < nodesCount; i++)*/
	/*graph__add_node(graph, nodesNames[i]);*/

	/*for(i=0; i < linksCount; i++)*/
	/*{*/
	/*int firstNodeindex = -1;*/
	/*int secondNodeIndex = -1;*/
	/*int j;*/

	/*for(j=0; j < nodesCount; j++)*/
	/*{*/
	/*if(!strcmp(linkLabels[i][0], nodesNames[i]))*/
	/*firstNodeindex = j;*/
	/*if(!strcmp(linkLabels[i][1], nodesNames[i]))*/
	/*secondNodeIndex = j;*/
	/*}*/
	/*Direction dir = graph__most_logical_direction_link(*/
	/*nodesPos[firstNodeindex], nodesPos[secondNodeIndex]);*/
	/*graph__add_link(graph, nodesNames[firstNodeindex], nodesNames[secondNodeIndex], dir);*/
	/*}*/

	/*printf("Finished loading %s\n", aquariumName);*/
}
void graph__clear(Graph* graph)
{
	if(graph)
	{
		Graph_node* currentNode = NULL;
		if(!graph__is_empty(graph))
		{
			graph__for_each_node(graph, currentNode)
			{
				list__remove_on_for_each(graph->nodes);
			}
		}
	}
}
bool graph__node_exist(Graph* graph, char* name)
{
	Graph_node* node = NULL;
	node = graph__get_node(graph,name);

	if(node == NULL)
		return false;
	else
		return true;
}
