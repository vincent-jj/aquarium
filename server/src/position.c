#include "position.h"
#include <stdlib.h>

Position* position__create_position(int x, int y)
{
	Position* ret = malloc(sizeof(Position));
	if(!ret)
		return NULL;
	ret->x = x;
	ret->y = y;
}
Position* position__create_copy(Position* position)
{
	return  position__create_position(position->x, position->y);
}
void position__destroy(Position* position)
{
	free(position);
}
