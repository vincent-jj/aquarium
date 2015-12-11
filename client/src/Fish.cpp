#include "Fish.hpp"
#include <vector>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <math.h>
#include <map>
using namespace std;
extern SDL_Surface *background, *window;
extern map < string, SDL_Surface* > imgFish;
extern vector<Position> offsetWhenArrived;

void Fish::display(void)
{
	if(remainingTime > 0 || remainingStill > 0)
	{
		SDL_Surface *img = imgFish[imgPath];
		SDL_Rect pos;
		pos.x = (int)round(((double)window->w) * currentPosition.x/100);
		pos.y = (int)round(((double)window->h) * currentPosition.y/100);
		size.x = (double)(img->w*100)/window->w ;
		size.y = (double)(img->h*100)/window->h;
		SDL_BlitSurface(img, NULL, window, &pos);
	}
}
void Fish::writeConsole(void)
{
	cout << "Fish " << name << " at " << (int)currentPosition.x << "x" << (int)currentPosition.y << endl;
}
void Fish::update(void)
{
	if(remainingTime > 0)
	{
		currentPosition.x += deltaPosition.x;
		currentPosition.y += deltaPosition.y;
		--remainingTime;
	}
	else if(remainingStill > 0)
	{
		currentPosition.x += offsetWhenArrived[remainingStill%offsetWhenArrived.size()].x;
		currentPosition.y += offsetWhenArrived[remainingStill%offsetWhenArrived.size()].y;
	
		--remainingStill;
	}
}
