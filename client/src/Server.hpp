#pragma once
#include <string>
#include <list>
#include <mutex>
using namespace std;

class Server
{
	string ip, nameNode;
	unsigned short port;
	int socketServer = -1;
	list<string> msgSent;
	mutex msgSentMutex;
	public:
	Server(string ip, unsigned short port, string nm);
	void getMessage(void);
	void ping(void);
	void getFishes(void);
	void getFishesContinuously(void);
	void sendMsg(string msg);
	bool isConnected(void);
	~Server(void);
	private:
	bool hello(string name);
	bool readCommand(string & command);
	void printLastCommand(bool stat);
	void updateFishes(string command);
	string getPathImg(string nameFish);
};
