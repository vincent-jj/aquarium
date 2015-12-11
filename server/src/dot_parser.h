#pragma once

#include "couple.h"

#define MAX_WORD_LENGTH 100



int dot_parser__f_get_next_block(FILE* fp, char* text);
bool dot_parser__s_get_label(char* src, char* label);
Couple dot_parser__s_get_pos(char* src);
bool dot_parser__f_get_aquarium_name(FILE* fp, char* aquariumName);
int dot_parser__f_is_node_or_link(FILE* fp, char nodes[2][MAX_WORD_LENGTH]);
void dot_parser__f_get_node(FILE* fp, char* label, Couple* pos);
void dot_parser__f_get_link(FILE* fp, char* label);
