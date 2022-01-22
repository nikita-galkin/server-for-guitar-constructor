#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <winsock.h>
#pragma comment(lib, "ws2_32.lib")
#include <mysql.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <list>
#include <ctime>
#include <fstream>
#include <thread>
#include <map>

class DB {
public:
	static bool start();
	static MYSQL_RES* sendReq(std::string req);
	static auto mysqlError();
	static void stop();
private:
	static MYSQL* connection;
	static std::string host;
	static unsigned int port;
	static std::string user;
	static std::string password;
	static std::string dbname;
};
