#include <iostream>
#include <thread>
#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <mutex>
#include <regex>
#include <unistd.h>
#include <dirent.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>
#include <time.h>
#include "Fish.hpp"
#include "Server.hpp"
#include "global.hpp"

using namespace std;

map < string, Fish * >allFishes;
mutex allFishesMutex;
Server *server = NULL;
bool running = true;
string ip = "127.0.0.1";
unsigned short port = 3300;
string nodeName = "";
string ressourcesPath = "img";
int ping_freq = 1;
vector < string > nameFish;
map < string, SDL_Surface* > imgFish;
int idxNameFish = 0;
SDL_Surface *window = NULL;
SDL_Surface *background = NULL;
vector<Position> offsetWhenArrived;

void parseLineConfigFile(string line)
{
	if(line.substr(0, strlen(CONFIG_PORT)) == CONFIG_PORT)
	{
		if(sscanf(line.c_str(), CONFIG_PORT " = %hu", &port) != 1)
			printf("Config file expected : " CONFIG_PORT " = <nb>\n");
		cout << "Port use : " << ping_freq << " second(s)" << endl;
	}
	else if(line.substr(0, strlen(CONFIG_DISPLAY_TIMEOUT)) == CONFIG_DISPLAY_TIMEOUT)
	{
		if(sscanf(line.c_str(), CONFIG_DISPLAY_TIMEOUT " = %d", &ping_freq) != 1)
			printf("Config file expected : " CONFIG_DISPLAY_TIMEOUT " = <nb>\n");
		cout << "Send one ping every " << ping_freq << " second(s)" << endl;
	}
	else if(line.substr(0, strlen(CONFIG_IP)) == CONFIG_IP)
	{
		ip = line.substr(strlen(CONFIG_IP) + 3);
		cout << "Ip target " << ip << endl;
	}
	else if(line.substr(0, strlen(CONFIG_ID)) == CONFIG_ID)
	{
		nodeName = line.substr(strlen(CONFIG_ID) + 3);
		cout << "Node wanted " << nodeName << endl;
	}
	else if(line.substr(0, strlen(CONFIG_RESSOURCES)) == CONFIG_RESSOURCES)
	{
		ressourcesPath = line.substr(strlen(CONFIG_RESSOURCES) + 3);
		cout << "Ressources directory" << nodeName << endl;
	}
}

void loadConfigFile(string filename)
{
	std::ifstream file(filename);
	if(!file.is_open())
		return;
	cout << "Load config file : " << filename << endl;
	string line;

	while(getline(file, line))
	{
		//If it begin with a #, it's a comment
		if(line[0] != '#' && line[0] != '\n')
			parseLineConfigFile(line.c_str());
	}
	file.close();
}

void status(void)
{
	//Commande status
	allFishesMutex.lock();
	cout << "Connecté au contrôleur, " << allFishes.size() << " poisson(s) trouvé(s)" << endl;
	  for(auto fish : allFishes)
	{
		fish.second->writeConsole();
	}
	allFishesMutex.unlock();
}

void readStdin(void)
{
	//Lit l'entrée standard. Sans timeout, c'est pourquoi l'on doit appuyer sur entrée à la fin.
	string line;
	while(running)
	{
		getline(cin, line);
		if(line == "status")
			status();
		else if(line.substr(0, 7) == "addFish" || line.substr(0, 9) == "startFish" || line.substr(0, 7) == "delFish")
			server->sendMsg(line);
		else if(line.substr(0, 21) == "getFishesContinuously")
			server->getFishesContinuously();
		else if(line.substr(0, 9) == "getFishes")
			server->getFishes();
		else if(line.substr(0, 7) == "log out")
		{
			server->sendMsg(line);
			break;
		}
		else
			cout << "NOK : commande introuvable" << endl;
		line.clear();
	}
}

void sendPing(void)
{
	//Envois un ping toutes les ping_freq secondes
	int i = 0;
	while(running)
	{
		if(i == 0)
			server->ping();
		sleep(1);
		i = (i + 1) % ping_freq;
	}
}

void display(void)
{
	//Affiche régulièrement tous les poissons du client
	SDL_Rect backgroundPos;
	backgroundPos.x = 0;
	backgroundPos.y = 0;
	while(running)
	{
		SDL_BlitSurface(background, NULL, window, &backgroundPos);
		allFishesMutex.lock();
		for(auto fish:allFishes)
		{
			fish.second->display();
			fish.second->update();
		}
		allFishesMutex.unlock();
		SDL_Flip(window);
		usleep(1000000 / FREQ_REFRESH_SCREEN);	//TODO revoir temps sleep display
	}
}

void genRandom(string & s, const int len)
{
	static const char alphanum[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	for(int i = 0; i < len; ++i)
		s.push_back(alphanum[rand() % (sizeof(alphanum) - 1)]);
}

void delFishAt(int x, int y)
{
	//Fonction de supression d'un poisson lors d'un clique droit
	x = ((x * 100) / window->w);
	y = ((y * 100) / window->h);

	Fish *f = NULL;
	allFishesMutex.lock();
	for(auto fish:allFishes)
	{
		if(x >= fish.second->currentPosition.x && x < fish.second->currentPosition.x + fish.second->size.x && y >= fish.second->currentPosition.y && y < fish.second->currentPosition.y + fish.second->size.y)
		{
			string command = "delFish " + fish.second->name;
			server->sendMsg(command);
			f = fish.second;
			break;
		}
	}
	if(f)
	{
		allFishes.erase(f->name);
		delete f;
	}
	allFishesMutex.unlock();
	server->getFishes();
}

void addFishAt(int x, int y)
{
	//Fonction d'ajout d'un poisson lors d'un clique
	string first, randPart;
	first = nameFish[idxNameFish];
	genRandom(randPart, 10);
	stringstream ss;
	string name = first + "_" + randPart;
	ss << "addFish " << name << " at " << ((x * 100) / window->w) << "x" << ((y * 100) / window->h) << ", 2x2, RandomWayPoint";
	server->sendMsg(ss.str());
	ss.str("");
	ss << "startFish " << name;
	server->sendMsg(ss.str());
	server->getFishes();
}

void killThemAll(void)
{
	allFishesMutex.lock();
	for(auto fish:allFishes)
	{
		string command = "delFish " + fish.second->name;
		server->sendMsg(command);
		delete fish.second;
	}
	allFishes.clear();
	allFishesMutex.unlock();
	server->getFishes();
}

void getEvent(void)
{
	//Écoute les événements de la sdl
	SDL_Event event;
	while(running)
	{
		SDL_WaitEvent(&event);
		switch (event.type)
		{
		case SDL_KEYDOWN:
			switch (event.key.keysym.sym)
			{
			case SDLK_ESCAPE:	/* Appui sur la touche Echap, on arrête le programme */
				running = false;
				server->sendMsg("log out");
				break;
			case SDLK_g:
				server->getFishes();
				break;
			case SDLK_c:
				server->getFishesContinuously();
				break;
			case SDLK_k:
				killThemAll();
				break;
			default:
				break;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			//event.button.x
			if(event.button.button == SDL_BUTTON_RIGHT)
				delFishAt(event.button.x, event.button.y);
			else if(event.button.button == SDL_BUTTON_LEFT)
				addFishAt(event.button.x, event.button.y);
			else if(event.button.button == SDL_BUTTON_WHEELUP)
			{
				idxNameFish = (idxNameFish + 1) % nameFish.size();
				cout << "Wheel fish : " << nameFish[idxNameFish] << endl;
			}
			else if(event.button.button == SDL_BUTTON_WHEELDOWN)
			{
				idxNameFish = (idxNameFish + nameFish.size() - 1) % nameFish.size();
				cout << "Wheel fish : " << nameFish[idxNameFish] << endl;
			}
			break;
		default:
			break;
		}
	}
}

void getMsg(void)
{
	//Écoute les messages du serveur et fait les mise à jour
	server->getMessage();
}

void initGraphic(void)
{
	window = SDL_SetVideoMode(800, 600, 32, SDL_DOUBLEBUF | SDL_HWSURFACE);
	if(window == NULL)
	{
		fprintf(stderr, "Impossible de charger le mode vidéo: %s\n", SDL_GetError());
		exit(EXIT_FAILURE);
	}

	SDL_Init(SDL_INIT_VIDEO);
	SDL_WM_SetCaption("Aquarium", NULL);

	background = SDL_LoadBMP("img/aquarium.bmp");
	SDL_Rect backgroundPos;
	backgroundPos.x = 0;
	backgroundPos.y = 0;
	SDL_BlitSurface(background, NULL, window, &backgroundPos);
}

void quitGraphic(void)
{
	SDL_FreeSurface(background);
	SDL_Quit();
}

void getAllRessourcesForFish(void)
{
	//Ajoute les fichier de ressourcesPath dans le tableau qui permettra d'ajouter des poissons en cliquant
	DIR *dir;
	struct dirent *ent;
	if((dir = opendir(ressourcesPath.c_str())) != NULL)
	{
		while((ent = readdir(dir)) != NULL)
		{
			string name = string(ent->d_name).substr(0, strlen(ent->d_name) - 4);
			if(!name.empty() && name != "aquarium" && name != "." && name != "..")
			{
				nameFish.push_back(name);
				imgFish[ressourcesPath + "/" + name + ".png"] = IMG_Load(string(ressourcesPath + "/" + name + ".png").c_str());
			}
		}
		closedir(dir);
	}
	else
	{
		cout << "No png files in " << ressourcesPath << endl;
	}
}

int main()
{
	srand(time(NULL));
	offsetWhenArrived.push_back({.x = 0, .y= -1.0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = 1.0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = 1.0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 1.0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 1.0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = -1.0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = -1.0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	offsetWhenArrived.push_back({.x = 0, .y= -1.0});
	offsetWhenArrived.push_back({.x = 0, .y= 0});
	loadConfigFile("affichage.cfg");
	getAllRessourcesForFish();
	while(server == NULL)
	{
		server = new Server(ip, port, nodeName);
		if(!server->isConnected())
		{
			delete server;
			server = NULL;
		}
	}
	thread ping(sendPing);
	thread std_in(readStdin);
	thread listener(getMsg);
	initGraphic();
	thread screen(display);
	thread event(getEvent);

	listener.join();
	ping.join();
	screen.join();
	event.join();
	quitGraphic();
	cout << "Please press enter ...";
	cout.flush();
	std_in.join();
	delete server;
  for(auto fish_p:allFishes)
		delete fish_p.second;
	return 0;
}
