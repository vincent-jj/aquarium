#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "../src/dot_parser.h"
#include "ctest.h"

#define MAX_WORD_LENGTH 100
#define MAX_AQUARIUM_DIM 30


CTEST(dot_parser, f_getNextBlock)
{
    FILE * fp;
    char text[MAX_WORD_LENGTH] = "";
    char next_char = 0;
    fp = fopen("tst/parsing-test-files/get_next_block.txt", "r");
    if(fp != NULL)
    {
        dot_parser__f_get_next_block(fp, text);
        next_char = fgetc(fp);
    }
    ASSERT_STR(text, "[inside brackets");
    ASSERT_EQUAL(next_char, 'a');
    fclose(fp);
}
CTEST(dot_parser, s_get_label)
{
    char* text = "blablabla label=\"datlabel\" blablabla";
    char label[MAX_WORD_LENGTH] = "";
    bool is_catched = false;

    is_catched = dot_parser__s_get_label(text, label);
    ASSERT_STR("datlabel", label);
    ASSERT_TRUE(is_catched);
}
CTEST(dot_parser, s_get_pos)
{
    char* text = "blablabla pos=\"7, 3!\" blablabla";

    Couple pos = (Couple) {-1, -1};
    pos = dot_parser__s_get_pos(text);
    ASSERT_EQUAL(7, pos.x);
    ASSERT_EQUAL(3, pos.y);
}
CTEST(dot_parser, f_get_aquarium_name)
{
    FILE * fp;
    char text[MAX_WORD_LENGTH] = "";
    char next_char = 0;
    fp = fopen("tst/parsing-test-files/get_aquarium_name.txt", "r");
    if(fp != NULL)
    {
        dot_parser__f_get_aquarium_name(fp, text);
        next_char = fgetc(fp);
    }
    ASSERT_STR(text, "Aquarium name ");
    ASSERT_EQUAL(next_char, '{');
    fclose(fp);
}
CTEST(dot_parser, f_is_node_or_link)
{
    FILE * fp_node, *fp_link;
    char text_node[2][MAX_WORD_LENGTH];
    char text_link[2][MAX_WORD_LENGTH];
    int is_node = -1;
    int is_link = -1;

    fp_node = fopen("tst/parsing-test-files/node.txt", "r");
    fp_link = fopen("tst/parsing-test-files/link.txt", "r");
    if(fp_node != NULL)
    {
        is_node = dot_parser__f_is_node_or_link(fp_node, text_node);
        is_link = dot_parser__f_is_node_or_link(fp_link, text_link);
    }
    ASSERT_STR(text_node[0], "node");
    ASSERT_STR(text_link[0], "node1");
    ASSERT_STR(text_link[1], "node2");
    ASSERT_EQUAL(is_node, 0);
    ASSERT_EQUAL(is_link, 1);
    fclose(fp_node);
    fclose(fp_link);
}
CTEST(dot_parser, f_get_node)
{
    FILE * fp;
    char label[MAX_WORD_LENGTH] = "";
    Couple pos = (Couple){-1, -1};

    fp = fopen("tst/parsing-test-files/node.txt", "r");
    if(fp != NULL)
    {
        dot_parser__f_get_node(fp, label, &pos);
    }
    ASSERT_STR("1", label);
    ASSERT_EQUAL(5, pos.x);
    ASSERT_EQUAL(5, pos.y);
    fclose(fp);
}
CTEST(dot_parser, f_get_link)
{
    FILE * fp;
    char label[MAX_WORD_LENGTH] = "";

    fp = fopen("tst/parsing-test-files/link.txt", "r");
    if(fp != NULL)
    {
        dot_parser__f_get_link(fp, label);
    }
    ASSERT_STR("3", label);
    fclose(fp);
}
