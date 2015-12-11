#include "Server.hpp"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <regex>
#include <list>
#include <map>
#include "Fish.hpp"
#include "global.hpp"

using namespace std;

extern map<string, Fish*> allFishes;
extern mutex allFishesMutex;
extern bool running;
extern string ressourcesPath;
std::regex rege ("\\[([a-zA-Z_]+) at ([0-9]+)x([0-9]+),([0-9]+)x([0-9]+),([0-9]+)\\]");   // matches words beginning by "sub"

Server::Server(string ip, unsigned short port, string nm)
{
    struct sockaddr_in sin = { 0 }; /* initialise la structure avec des 0 */
	socketServer = socket(AF_INET, SOCK_STREAM, 0);
    sin.sin_port = htons(port); /* on utilise htons pour le port */
    sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = inet_addr(ip.c_str());
	int err;
	do
	{
		err = connect(socketServer, (struct sockaddr*)&sin, sizeof(sin));
		if(err == 0)
			break;
		sleep(1);
	}while(err != 0);
	if(!hello(nm))
	{
		close(socketServer);
		socketServer = -1;
		cout << "No node available" << endl;
	}
	else
		cout << "Greeting " << nameNode << endl;
}
void Server::getMessage(void)
{
	string command;
	while(running)
	{
		if(readCommand(command))
		{
			if(command == "bye")
				running = false;
			else if(command == "OK")
				printLastCommand(true);
			else if(command == "NOK")
				printLastCommand(false);
			else if(command.find("list ") == 0)
				updateFishes(command.substr(5));
			else if(command.find("pong") != 0)
				cout << "Error : " << command << endl;
		}
		command.clear();
	}
}
void Server::ping(void)
{
	int nb = rand()%4;
	switch(nb)
	{
		case 0:
		send(socketServer, "ping loutre\n", strlen("ping loutre\n"), 0);	
		break;
		case 1:
		send(socketServer, "ping canard\n", strlen("ping canard\n"), 0);	
		break;
		case 2:
		send(socketServer, "ping phoque\n", strlen("ping phoque\n"), 0);	
		break;
		case 3:
		send(socketServer, "ping licorne\n", strlen("ping licorne\n"), 0);	
		break;
	}
}
void Server::getFishes(void)
{
	send(socketServer, "getFishes\n", strlen("getFishes\n"), 0);	
}
void Server::getFishesContinuously(void)
{
	send(socketServer, "getFishesContinuously\n", strlen("getFishesContinuously\n"), 0);	
}
bool Server::hello(string name)
{
	string msg;
	if(name.empty())
		msg = "hello\n";
	else
		msg = "hello in as " + name + "\n";
	send(socketServer, msg.c_str(), msg.size(), 0);	
	msg.clear();
	readCommand(msg);
	if(msg == "no greeting")
		return false;
	if(msg.empty())
		return false;
	if(msg.substr(0, 9) == "greeting ")
		nameNode = msg.substr(9);
	return true;	
}
void Server::sendMsg(string msg)
{
	msgSentMutex.lock();
	msgSent.push_back(msg);
	msgSentMutex.unlock();
	msg = msg + "\n";
	send(socketServer, msg.c_str(), msg.size(), 0);	
}
bool Server::readCommand(string & command)
{
	char letter[3] = {0};
	while(letter[0] != '\n')
	{
		int size = recv(socketServer, letter, 1, 0);
		if(size == 0)
		{
			command = "bye";
			break;
		}
		if(letter[0] != '\n')
			command = command + letter[0];
	}
	return true;
}
void Server::printLastCommand(bool stat)
{
	msgSentMutex.lock();
	string com = msgSent.front();
	msgSent.pop_front();
	msgSentMutex.unlock();
	cout << com << " " << (stat ? "OK" : "NOK") << endl;
}
void Server::updateFishes(string command)
{
	cout << "List -> " << command << endl;
	std::smatch m;

	list<Fish*> tmpFishes;
	Fish* tmp;
	double x_dst, y_dst;
	while (std::regex_search (command,m,rege))
	{
		tmp = new Fish;
		string x_p = m[2];
		string y_p = m[3];
		string x_g = m[4];
		string y_g = m[5];
		string tm = m[6];
		tmp->name = m[1];
		tmp->currentPosition.x = atof(x_p.c_str());
		tmp->currentPosition.y = atof(y_p.c_str());
		x_dst = atof(x_g.c_str());
		y_dst = atof(y_g.c_str());
		tmp->remainingTime = (atoi(tm.c_str()) - 1) * FREQ_REFRESH_SCREEN;
		tmp->remainingStill = FREQ_REFRESH_SCREEN;
		tmp->deltaPosition.x = (x_dst - tmp->currentPosition.x) / (double) tmp->remainingTime;
		tmp->deltaPosition.y = (y_dst - tmp->currentPosition.y) / (double) tmp->remainingTime;
		tmp->imgPath = getPathImg(tmp->name);
		tmpFishes.push_front(tmp);
		command = m.suffix().str();
	}

	allFishesMutex.lock();
	for(Fish* new_fish : tmpFishes)
	{
		if(allFishes.count(new_fish->name) == 0)
			allFishes[new_fish->name] = new_fish;
		else
		{
			auto tmp = allFishes[new_fish->name];
			allFishes[new_fish->name] = new_fish;
			delete tmp;
		}
	}
	for(auto it = allFishes.begin(); it != allFishes.end();)
		if(it->second->remainingTime == 0)
			allFishes.erase(it++);
		else
			++it;
	allFishesMutex.unlock();
}
bool Server::isConnected(void)
{
	return socketServer != -1;
}
Server::~Server(void)
{
	if(socketServer > 0)
		close(socketServer);
}
string Server::getPathImg(string nameFish)
{
	int pos = nameFish.find("_");
	nameFish = nameFish.substr(0, pos);
	string path = ressourcesPath + "/" + nameFish +".png";
	std::ifstream ifs;
	ifs.open(path);
	if (ifs.is_open())
	{
		ifs.close();
		return path;
	}
	return ressourcesPath + "/default.png";
}
