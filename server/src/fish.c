#include "fish.h"
#include <stdlib.h>
#include <string.h>
#include "position.h"

Fish* fish__create_fish(char* name, Position* position, Position* size, char* mobility_name)
{
	Fish* ret = malloc(sizeof(Fish));
	if(ret == NULL)
		return NULL;
	ret->name = strdup(name);
	if(ret->name == NULL)
	{
		free(ret);
		return NULL;
	}
	ret->mobility_name = strdup(mobility_name);
	if(ret->mobility_name == NULL)
	{
		free(ret->name);
		free(ret);
		return NULL;
	}
	ret->position = position;
	ret->size = size;
	ret->started = false;
	ret->remaining_time_on_client = 0;
	ret->goal = NULL;
	return ret;
}
Position* fish__get_size(Fish* fish)
{
	return fish->size;
}
char* fish__get_mobility(Fish* fish)
{
	return fish->mobility_name;
}
bool fish__is_started(Fish* fish)
{
	return fish->started;
}
void fish__set_started(Fish* fish, bool started)
{
	fish->started = started;
}
Position* fish__get_position(Fish* fish)
{
	return fish->position;
}
char* fish__get_name(Fish* fish)
{
	return fish->name;
}
void fish__destroy(Fish* fish)
{
	free(fish->name);
	free(fish->mobility_name);
	position__destroy(fish->position);
	position__destroy(fish->size);
	if(fish->goal == NULL)
		position__destroy(fish->goal);
	free(fish);
}
void fish__set_remaining_time_on_client(Fish* fish, int value)
{
	fish->remaining_time_on_client = value;
}
void fish__decrease_remaining_time_on_client(Fish* fish, int value)
{
	fish->remaining_time_on_client -= value;
}
int fish__get_remaining_time_on_client(Fish* fish)
{
	return fish->remaining_time_on_client;
}
Position* fish__get_goal(Fish* fish)
{
	return fish->goal;
}
void fish__set_goal(Fish* fish, Position* goal)
{
	if(fish->goal != NULL)
		position__destroy(fish->goal);
	fish->goal = goal;
}
void fish__set_position(Fish* fish, Position* position)
{
	if(fish->position != NULL)
		position__destroy(fish->position);
	fish->position = position;
}
