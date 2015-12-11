#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "ctest.h"
#include "../src/graph.h"


CTEST(graph, create_node)
{
    Graph_node* node = graph__create_node("name");
	ASSERT_STR("name", node->name);
}
CTEST(graph, set_node_neighbor)
{
    Direction n1Side = LEFT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, node1->neighbors[2]->name);
}
CTEST(graph, node_up_to)
{
    Direction n1Side = UP;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, graph__node_up_to(node1)->name);
}
CTEST(graph, node_down_to)
{
    Direction n1Side = DOWN;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, graph__node_down_to(node1)->name);
}
CTEST(graph, node_left_to)
{
    Direction n1Side = LEFT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, graph__node_left_to(node1)->name);
}
CTEST(graph, node_right_to)
{
    Direction n1Side = RIGHT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, graph__node_right_to(node1)->name);
}
CTEST(graph, node_next_to)
{
    Direction n1Side = LEFT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_STR(node2->name, graph__node_next_to(node1, n1Side)->name);
}
CTEST(graph, opposite_direction)
{
    ASSERT_EQUAL(LEFT, graph__opposite_direction(RIGHT));
    ASSERT_EQUAL(RIGHT, graph__opposite_direction(LEFT));
    ASSERT_EQUAL(UP, graph__opposite_direction(DOWN));
    ASSERT_EQUAL(DOWN, graph__opposite_direction(UP));
}
CTEST(graph, is_node_connected)
{
    Direction n1Side = LEFT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    Graph_node* node3 = graph__create_node("node3");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_TRUE(graph__is_node_connected(node1));
    ASSERT_FALSE(graph__is_node_connected(node2));
    ASSERT_FALSE(graph__is_node_connected(node3));
}
CTEST(graph, has_node_connected)
{
    Direction n1Side = LEFT;
    Graph_node* node1 = graph__create_node("node1");
    Graph_node* node2 = graph__create_node("node2");
    graph__set_node_neighbor(node1, n1Side, node2);
    ASSERT_TRUE(graph__has_node_connected(node1, n1Side));
    ASSERT_FALSE(graph__has_node_connected(node1, RIGHT));
    ASSERT_FALSE(graph__has_node_connected(node2, RIGHT));
}
CTEST(graph, most_logical_direction_link)
{
    Couple pos1 = (Couple){0, 0};
    Couple pos2 = (Couple){5, 5};
    Couple pos3 = (Couple){5, 10};
    Couple pos4 = (Couple){15, 0};
    ASSERT_EQUAL(RIGHT, graph__most_logical_direction_link(pos1, pos2));
    ASSERT_EQUAL(UP, graph__most_logical_direction_link(pos1, pos3));
    ASSERT_EQUAL(RIGHT, graph__most_logical_direction_link(pos1, pos4));
    ASSERT_EQUAL(LEFT, graph__most_logical_direction_link(pos2, pos1));
    ASSERT_EQUAL(UP, graph__most_logical_direction_link(pos2, pos3));
    ASSERT_EQUAL(RIGHT, graph__most_logical_direction_link(pos2, pos4));
    ASSERT_EQUAL(DOWN, graph__most_logical_direction_link(pos3, pos1));
    ASSERT_EQUAL(DOWN, graph__most_logical_direction_link(pos3, pos2));
    ASSERT_EQUAL(RIGHT, graph__most_logical_direction_link(pos3, pos4));
    ASSERT_EQUAL(LEFT, graph__most_logical_direction_link(pos4, pos1));
    ASSERT_EQUAL(LEFT, graph__most_logical_direction_link(pos4, pos2));
    ASSERT_EQUAL(LEFT, graph__most_logical_direction_link(pos4, pos3));
}

CTEST(graph, create)
{
    Graph* graph = graph__create();
    ASSERT_TRUE(graph__is_empty(graph));
    ASSERT_TRUE(graph != NULL);
}
CTEST(graph, add_node)
{
    Graph* graph = graph__create();
    ASSERT_EQUAL(0, list__get_size(graph->nodes));
    graph__add_node(graph, "node1");
    ASSERT_EQUAL(1, list__get_size(graph->nodes));
    Graph_node* node = (Graph_node*) graph->nodes->head->data;
    ASSERT_STR("node1", node->name);

}
CTEST(graph, del_node)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    ASSERT_EQUAL(1, list__get_size(graph->nodes));
    graph__del_node(graph, "node1");
    ASSERT_EQUAL(0, list__get_size(graph->nodes));
}
CTEST(graph, add_link)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__add_node(graph, "node2");
    graph__add_link(graph, "node1", "node2", LEFT);
    Graph_node* node1 = graph__get_node(graph, "node1");
    Graph_node* node2 = graph__get_node(graph, "node2");
    ASSERT_STR(node2->name, graph__node_left_to(node1)->name);
    ASSERT_STR(node1->name, graph__node_right_to(node2)->name);
}
CTEST(graph, del_link)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__add_node(graph, "node2");
    graph__add_link(graph, "node1", "node2", LEFT);
    graph__del_link(graph, "node1", "node2", LEFT);
    Graph_node* node1 = graph__get_node(graph, "node1");
    ASSERT_NULL(graph__node_left_to(node1));
}
CTEST(graph, get_node)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__add_node(graph, "node2");
    Graph_node* node1 = graph__get_node(graph, "node1");
    Graph_node* node2 = graph__get_node(graph, "node2");
    ASSERT_STR("node1", node1->name);
    ASSERT_STR("node2", node2->name);
}
CTEST(graph, is_empty)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__add_node(graph, "node2");
    graph__del_node(graph, "node1");
    graph__del_node(graph, "node2");
    ASSERT_TRUE(graph__is_empty(graph));
}
CTEST(graph, load)
{
    Graph* graph = graph__create();
    FILE * fp;
    graph__load(graph, "tst/parsing-test-files/load.dot");
    Graph_node* n1 = graph__get_node(graph, "n1");
    Graph_node* n2 = graph__get_node(graph, "n2");
    Graph_node* n3 = graph__get_node(graph, "n3");
    Graph_node* n4 = graph__get_node(graph, "n4");

    ASSERT_STR("n3", graph__node_up_to(n1)->name);
    // Bizarrerie, mais c'est comme ça
    // On voulait des positions relatives
    // C'est le prix à payer
    // Un noeud ne peut pas avoir deux noeuds sur un même côté
    // Faudra juste bien construire le graphe
    ASSERT_STR("n4", graph__node_down_to(n3)->name);

    ASSERT_STR("n2", graph__node_right_to(n1)->name);
    ASSERT_STR("n1", graph__node_left_to(n2)->name);

    ASSERT_STR("n4", graph__node_up_to(n2)->name);
    ASSERT_STR("n2", graph__node_down_to(n4)->name);

    ASSERT_STR("n3", graph__node_up_to(n4)->name);
    ASSERT_STR("n4", graph__node_down_to(n3)->name);
}
CTEST(graph, save)
{
}
CTEST(graph, clear)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__clear(graph);
    ASSERT_EQUAL(0, list__get_size(graph->nodes));
}
CTEST(graph, node_exist)
{
    Graph* graph = graph__create();
    graph__add_node(graph, "node1");
    graph__add_node(graph, "node2");
    ASSERT_TRUE(graph__node_exist(graph, "node1"));
    ASSERT_TRUE(graph__node_exist(graph, "node2"));
    ASSERT_FALSE(graph__node_exist(graph, "node3"));
}
