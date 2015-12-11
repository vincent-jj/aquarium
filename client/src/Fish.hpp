#pragma once
#include <string>
#include "Position.hpp"
#include <SDL/SDL.h>

using namespace std;

class Fish
{
	public:
	int remainingTime, remainingStill;
	Position currentPosition, deltaPosition, size;
	
	string imgPath;
	string name;
	void display(void);
	void update(void);
	void writeConsole(void);
};
