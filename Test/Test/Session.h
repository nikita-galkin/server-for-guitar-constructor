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
#define LOG(msg) { Session::Log << "[" << time(0) << "]" << msg; Session::Log.flush(); }
//u can use it only in server()
#define DEATH(msg, exitCode) { Session::Log << "[" << time(0) << "]" << msg; Session::Log.flush(); Session::Log.close(); DB::stop(); return exitCode; }

class Session
{
public:
	/// <summary>
	/// Creates a session for a client on a socket
	/// </summary>
	/// <param name="Sock">Client socket</param>
	/// <param name="timeToLife">Maximum session time in seconds</param>
	Session(SOCKET Sock, time_t timeToLife = 5 * 60 * 60);
	~Session()
	{
		LOG("Session destructed sockID = " << sock << std::endl);
		if (sock) closesocket(sock);
		doWork.join();
	}
	bool IsTimeUp();
	static std::ofstream Log;
	std::thread doWork;
private:
	SOCKET sock = 0;
	time_t timeStart;
	time_t ttl;
};

class DB {
public:
	/// <summary>
	/// Initialize and start sql connection
	/// </summary>
	/// <returns>False - error</returns>
	static bool start();
	/// <summary>
	/// Send sql request
	/// </summary>
	/// <param name="req">String with sql reqest</param>
	/// <returns>NULL on error</returns>
	static MYSQL_RES* sendReq(std::string req);
	/// <summary>
	/// Returns information about a database error
	/// </summary>
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
int server();
void session(SOCKET &sock);
void timer(std::list<Session*> &sessions, bool &exit);
bool sendTable(SOCKET &sock, std::string select, std::string startSending);
