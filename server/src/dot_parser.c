#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "dot_parser.h"
#include "list.h"
#include "err_exit.h"
#include "couple.h"

#define MAX_WORD_LENGTH 100
#define MAX_AQUARIUM_DIM 30


// Put the content of the text between the next pair of brackets in the string
// Return the length of the text between brackets
int dot_parser__f_get_next_block(FILE* fp, char* text)
{
    int index = 0;
    char c;

    do
    {
        c = fgetc(fp);
        text[index++] = c;
    }
    while(c != EOF && c != ']');

    text[index - 1] = 0;
    return index;
}

// Put the next label in the string
// Return a boolean if the label have been read
bool dot_parser__s_get_label(char* src, char* label)
{
    int i = 0;
    int isRead = 0;
    while(i < MAX_WORD_LENGTH && !isRead)
    {
        if(i > 7 &&
                src[i] == '"' &&
                src[i-1] == '=' &&
                src[i-2] == 'l' &&
                src[i-3] == 'e' &&
                src[i-4] == 'b' &&
                src[i-5] == 'a' &&
                src[i-6] == 'l')
        {
            i++;
            int labelIndex = 0;
            while(src[i] != '"')
            {
                label[labelIndex] = src[i];
                i++;
                labelIndex++;
                if(i == MAX_WORD_LENGTH)
                    err_exit("Misformed aquarium file, doublequote openned but not closed");
            }
            isRead = 1;
        }
        i++;
    }
    /*int stringsCatched =sscanf(src, "%*[a-zA-Z ]%*[ ]label=\"%[^\"]\"%*s", label);*/
    /*if(stringsCatched == 1)*/
    /*return true;*/
    /*else*/
    /*return false;*/
}

// Return a couple of ints
Couple dot_parser__s_get_pos(char* src)
{
    char tempPosStr[MAX_AQUARIUM_DIM] = "";
    int i = 0;
    int isRead = 0;
    while(i < MAX_WORD_LENGTH && !isRead)
    {
        if(i > 4 &&
                src[i] == '"' &&
                src[i-1] == '=' &&
                src[i-2] == 's' &&
                src[i-3] == 'o' &&
                src[i-4] == 'p')
        {
            i++;
            int posIndex = 0;
            while(src[i] != '"')
            {
                tempPosStr[posIndex] = src[i];
                i++;
                posIndex++;
                if(i == MAX_WORD_LENGTH)
                    err_exit("Misformed aquarium file, doublequote openned but not closed");
            }
            isRead = 1;
        }
        i++;
    }
    Couple c = {-1, -1};
    sscanf(tempPosStr, "%d, %d!", &c.x, &c.y);
    return c;
}

// Return true if the aquarium name has been read
bool dot_parser__f_get_aquarium_name(FILE* fp, char* aquariumName)
{
    char rawAquariumName[MAX_WORD_LENGTH] = "";
    int index = 0;
    char c;
    do
    {
        c = fgetc(fp);
        rawAquariumName[index++] = c;
    }
    while(c != EOF && c != '{');

    fseek(fp, -1, SEEK_CUR); // Because the opening brace is consumed and we don't want to

    int stringsCatched = sscanf(rawAquariumName, "graph %[^{]", aquariumName);
    if(stringsCatched == 1)
        return true;
    else
        return false;
}

// Return 0 if node, 1 if link, -1 if EOF, and store the node/nodes name(s)
int dot_parser__f_is_node_or_link(FILE* fp, char nodes[2][MAX_WORD_LENGTH])
{
    char rawNodeOrLinkName[MAX_WORD_LENGTH] = "";
    char nodeOrLinkName[MAX_WORD_LENGTH] = "";
    int index = 0;
    char c;

    do
    {
        c = fgetc(fp);
        rawNodeOrLinkName[index++] = c;
    }
    while(c != EOF && c != '[');

    if(c == EOF)
        return -1;

    sscanf(rawNodeOrLinkName, "%*s %[^[]", nodeOrLinkName);
    int stringsCatched = sscanf(nodeOrLinkName, "%s -- %[^[ ]", nodes[0], nodes[1]);

    if(stringsCatched == 1)
        return 0;
    else if(stringsCatched == 2)
        return 1;
    else
        err_exit("Aquarium file not valid");
    return 0; // Just to avoid warning
}

// Get every infos on a node
void dot_parser__f_get_node(FILE* fp, char* label, Couple* pos)
{
    char block[MAX_WORD_LENGTH] = "";

    dot_parser__f_get_next_block(fp, block);
    dot_parser__s_get_label(block, label);
    *pos = dot_parser__s_get_pos(block);

}

// Get every infos on a link
void dot_parser__f_get_link(FILE* fp, char* label)
{
    char block[MAX_WORD_LENGTH] = "";

    dot_parser__f_get_next_block(fp, block);
    dot_parser__s_get_label(block, label);
}
