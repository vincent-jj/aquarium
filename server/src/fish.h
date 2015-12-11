#pragma once
#include <stdbool.h>
struct Position;
typedef struct Position Position;

struct Fish
{
	Position* position, *size, *goal;
	char* name;
	char* mobility_name;
	bool started;
	int remaining_time_on_client;
};
typedef struct Fish Fish;

Fish* fish__create_fish(char* name, Position* position, Position* size, char* mobility_name);
Position* fish__get_position(Fish* fish);
Position* fish__get_size(Fish* fish);
char* fish__get_name(Fish* fish);
char* fish__get_mobility(Fish* fish);
bool fish__is_started(Fish* fish);
void fish__destroy(Fish* fish);
void fish__set_remaining_time_on_client(Fish* fish, int value);
void fish__set_position(Fish* fish, Position* position);
void fish__decrease_remaining_time_on_client(Fish* fish, int value);
Position* fish__get_goal(Fish* fish);
void fish__set_goal(Fish* fish, Position* goal);
int fish__get_remaining_time_on_client(Fish* fish);
