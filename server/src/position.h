#pragma once

struct Position
{
	int x;
	int y;
};
typedef struct Position Position;

Position* position__create_position(int x, int y);
Position* position__create_copy(Position* position);
void position__destroy(Position* position);
