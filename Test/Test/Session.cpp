#include "Session.h"
/**********************DB***********************/
MYSQL* DB::connection;
std::string DB::host = "localhost";
unsigned int DB::port = 3306;
std::string DB::user = "root";
std::string DB::password = "12345";
std::string DB::dbname = "basa";
int send(SOCKET sock, char* buff, unsigned int size)
{
	if (send(sock, (char*)&size, sizeof(size), 0) == -1) { LOG("Error in send size\n"); return -1; }
	return (send(sock, buff, size, 0) == -1);
}
int recv(SOCKET sock, char* buff, int size)
{
	unsigned int Size;
	if (recv(sock, (char*)&Size, sizeof(Size), 0) == -1) { LOG("Error in recv size\n"); return -1; }
	if (Size > size) { LOG("Error in recv size was very big\n"); return -1; }
	recv(sock, buff, Size, 0);
	LOG("Size = " << Size << " String from client = " << buff << "\n");//DBG
	return 0;
}

bool DB::start() {
	connection = mysql_init(nullptr);
	if (mysql_real_connect(connection, host.c_str(), user.c_str(), password.c_str(), dbname.c_str(), port, nullptr, 0) == nullptr)
	{
		LOG(mysql_error(connection) << "Connection to database is failed\n");
		return false;
	}
	return true;
}
void DB::stop() { mysql_close(connection); }
auto DB::mysqlError() {	return mysql_error(connection); }
MYSQL_RES* DB::sendReq(std::string req) {
	if (mysql_query(connection, req.c_str())) return nullptr;
	return mysql_store_result(connection);
}
/************************************************/

/*********************Session********************/
std::ofstream Session::Log;
Session::Session(SOCKET Sock, time_t timeToLife)
{
	LOG("Session started\n");
	sock = Sock;
	ttl = timeToLife;
	timeStart = time(0);
	doWork = std::thread(session, std::ref(sock));
}
bool Session::IsTimeUp() { return (time(0) - timeStart > ttl) ? true : false; }
/************************************************/

bool sendTable(SOCKET &sock, std::string select, std::string startSending)
{
	MYSQL_RES* res = DB::sendReq(select.c_str());
	if (res == nullptr)	{ LOG(DB::mysqlError() << "Error in sending request\n"); return false; }
	if (send(sock, (char*)startSending.c_str(), startSending.length() + 1) == -1) { LOG("Error in sending sock msg in " << startSending << std::endl); return false; }
	int numFields = mysql_num_fields(res);
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != nullptr) {
		std::stringstream  sstr;//TODO Find faster solve
		sstr << (row[0] ? row[0] : "");
		for (int i = 1; i < numFields; i++) sstr << '|' << (row[i] ? row[i] : "");
		if (send(sock, (char*)sstr.str().c_str(), sstr.str().length() + 1) == -1) { LOG("Error in sending sock msg in " << startSending << std::endl); return false; }
	}
	if (send(sock, (char*)"ENDTABLE", 9) == -1) { LOG("Error in sending sock msg in " << startSending << std::endl); return false; }
	return true;
}

std::string getTable(int ID) {
	switch(ID) {
	case 1: return "SELECT Примерная_цена FROM Анкер WHERE Код=";
	case 2: return "SELECT Примерная_цена FROM Бридж WHERE Код=";
	case 3: return "SELECT Цена FROM Вид_сборки WHERE Код=";
	case 4: return "SELECT Примерная_цена FROM Материал_корпуса WHERE Код=";
	case 5: return "SELECT Примерная_цена FROM Струны WHERE Код=";
	case 6: return "SELECT Примерная_цена FROM Материал_грифа WHERE Код=";
	case 7: return "SELECT Цена FROM Покраска WHERE Код=";
	case 8: return "SELECT Примерная_цена FROM Электронная_начинка WHERE Код=";
	case 9: return "SELECT Примерная_цена FROM Колки WHERE Код=";
	case 10: return "SELECT Примерная_цена FROM Звукосниматель WHERE Код=";
	default: return "";
	}
}

std::string updateTable(int ID) {
	switch(ID) {
	case 1: return "UPDATE Заказ SET ФИО_клиента='";
	case 2: return "UPDATE Заказ SET Номер_телефона='";
	case 3: return "UPDATE Заказ SET Изображение_аэрографии='";
	case 4: return "UPDATE Заказ SET 3D_Модель='";
	default: return "";
	}
}

int GetStuffID()
{
	std::map<int, int> Stuff;
	MYSQL_ROW row;
	MYSQL_RES* res = DB::sendReq("SELECT Код FROM Сотрудники");
	if (res == nullptr)	{ LOG(DB::mysqlError() << "Error in sending request\n"); return false; }
	int min = 0, minID = 0, numFields = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res)) != nullptr) { LOG("ID" << atoi(row[0]) << "\n"); Stuff[atoi(row[0])] = 0; }
	res = DB::sendReq("SELECT Сотрудник, COUNT(*) FROM Заказ GROUP BY Сотрудник");
	if (res == nullptr)	{ LOG(DB::mysqlError() << "Error in sending request\n"); return false; }
	numFields = mysql_num_fields(res);
	while ((row = mysql_fetch_row(res)) != nullptr) if (row[0] != nullptr && row[1] != nullptr) Stuff[atoi(row[0])] = atoi(row[1]);
	for (auto i : Stuff) if (i.second<min) { min = i.second; minID = i.first; }
	return minID;
}


void session(SOCKET& sock)
{
	char comandBuff[3000];
	if (recv(sock, comandBuff, 3000) == -1) { LOG("Error in recv msg in Session\n"); return; }		//Get command to start or continue old session
	if (strncmp(comandBuff, "Continue", 8) == 0) goto Continue;	
	/******Load and send tables to client********/
	if (!sendTable(sock, "SELECT Код, Тип_анкера FROM Анкер", "Анкер|Код|Тип_анкера")) return;
	if (!sendTable(sock, "SELECT Код, Фирма, Модель FROM Бридж", "Бридж|Код|Фирма|Модель")) return;
	if (!sendTable(sock, "SELECT Код, Сборка FROM Вид_сборки", "Вид_сборки|Код|Сборка")) return;
	if (!sendTable(sock, "SELECT Код, Материал FROM Материал_корпуса", "Материал_корпуса|Код|Материал")) return;
	if (!sendTable(sock, "SELECT Код, Фирма, Модель FROM Струны", "Струны|Код|Фирма|Модель")) return;
	if (!sendTable(sock, "SELECT Код, Материал FROM Материал_грифа", "Материал_грифа|Код|Материал")) return;
	if (!sendTable(sock, "SELECT Код, Тип FROM Покраска", "Покраска|Код|Тип")) return;
	if (!sendTable(sock, "SELECT Код, Конфигурация FROM Электронная_начинка", "Электронная_начинка|Код|Конфигурация")) return;
	if (!sendTable(sock, "SELECT Код, Фирма, Модель FROM Колки", "Колки|Код|Фирма|Модель")) return;
	if (!sendTable(sock, "SELECT Код, Фирма, Модель FROM Звукосниматель", "Звукосниматель|Код|Фирма|Модель")) return;
	/*******************************************/
Continue:
	memset(comandBuff, 0, 3000);
	MYSQL_RES* res;
	MYSQL_ROW row;
	int tableID = 0, valueID = 0;
	/***********Calculate Order amount************/
	while (true) {
		if (recv(sock, comandBuff, 3000) == -1) { LOG("Error in recv msg in Session\n"); return; }	//Get command with TableID
		if (strncmp(comandBuff, "ENDCALC", 7) == 0) break;													//Check exit condition 
		sscanf_s(comandBuff, "%d|%d", &tableID, &valueID);												//Parse tableId ans
		std::string sqlString = getTable(tableID);													//Get sql request string to get cost from TableID
		if (sqlString == "") { if (send(sock, (char*)"-1", 3) == -1) { LOG("Error in send msg in Session\n"); return; } }//If uncknwn ID send error;															
		else {
			sqlString += std::to_string(valueID);														//Add Value ID to sql reqest
			res = DB::sendReq(sqlString.c_str());														//Send request
			if (res == nullptr) { LOG(DB::mysqlError() << "Error in sending request\n"); return; }
			if ((row = mysql_fetch_row(res)) != nullptr) {												//Get result
				std::stringstream ssRow;													
				ssRow << (row[0] ? row[0] : NULL);														//Write cost in stream
				if (send(sock, (char*)ssRow.str().c_str(), ssRow.str().length() + 1) == -1) { LOG("Error in send msg in Session\n"); return; } //Send stream to client
			}
			else if(send(sock, (char*)"-1", 3) == -1) { LOG("Error in send msg in Session\n"); return; }
		}
	}
	/********************************************/
	//TODO mutex for BD? SYV
	DB::sendReq("INSERT INTO Заказ(Дата_заказа) VALUES(NOW())");			//Gen new Order in Base with DateTime = NOW
	res = DB::sendReq("SELECT LAST_INSERT_ID() FROM Заказ");
	if (res == nullptr) { LOG(DB::mysqlError() << "Error in sending request\n"); return; }
	std::stringstream  ssOrderID;
	if ((row = mysql_fetch_row(res)) != nullptr) ssOrderID << (row[0] ? row[0] : NULL); //Get new Order ID
	else { LOG("Error in get ssOrderID in Session\n"); return; }
	char* dataBuff = new char[65535];
	while (true) {
		memset(comandBuff, 0, 3000);
		memset(dataBuff, 0, 65535);
		tableID = 0;
		if (recv(sock, comandBuff, 3000) == -1) { LOG("Error in recv msg in Session\n"); delete[] dataBuff;  return; } 		//Get command with id info to add in order
		if (strncmp(comandBuff, "ENDCLIENTINFO", 13) == 0) break;															//If comand to end insert info - go away
		sscanf_s(comandBuff, "%d", &tableID);																					//Else in comand contains ID to insert info
		if (tableID == 5) {																									//5 means that client send Description
			char* tempdataBuff = new char[16777215];																		//Descrtiption is MediumText = 16 777 215 bytes
			std::stringstream ssTemp;
			if (recv(sock, tempdataBuff, 16777215) == -1) { LOG("Error in recv msg in Session\n"); delete[] tempdataBuff;  delete[] dataBuff; return; } //recv Description
			ssTemp << "UPDATE Заказ SET Дополнительные_пожелания='" << tempdataBuff << "' WHERE Код=" << ssOrderID.str(); 	//Construct sql request string
			DB::sendReq(ssTemp.str().c_str());//send request
			delete[] tempdataBuff;
			continue;
		}
		if(recv(sock, dataBuff, 65535) == -1) { LOG("Error in recv msg in Session\n"); delete[] dataBuff; return; }			//If it isn't description, then it must be shorter, then 65 535 bytes
		if (tableID == 6) {
			int Kolk = 1, SoundGet = 1, Bridge = 1, Anker = 1, TypeBuild = 1, TypeGrif = 1, DecaMaterial = 1, Strings = 1, Colouring = 1, Electric = 1, Stuff = 1;
			sscanf_s(dataBuff, "%d|%d|%d|%d|%d|%d|%d|%d|%d|%d", 
				&Kolk, 
				&SoundGet, 
				&Bridge, 
				&Anker, 
				&TypeBuild, 
				&TypeGrif, 
				&DecaMaterial, 
				&Strings,
				&Colouring, 
				&Electric);
			std::stringstream ssTablesIDInOrder;
			Stuff = GetStuffID();
			ssTablesIDInOrder << "UPDATE Заказ SET Колки=" << Kolk
				<< ", Звукосниматели=" << SoundGet
				<< ", Бридж=" << Bridge
				<< ", Анкер=" << Anker
				<< ", Вид_сборки=" << TypeBuild
				<< ", Материал_грифа=" << TypeGrif
				<< ", Материал_корпуса=" << DecaMaterial
				<< ", Струны=" << Strings
				<< ", Покраска=" << Colouring
				<< ", Электронная_начинка=" << Electric
				<< ", Сотрудник=" << Stuff
				<< " WHERE Код=" << ssOrderID.str();
			DB::sendReq(ssTablesIDInOrder.str().c_str());
			continue;
		}
		std::stringstream ssINFO;
		ssINFO << updateTable(tableID) << dataBuff << "' WHERE Код=" << ssOrderID.str(); //Get sql request string
		DB::sendReq(ssINFO.str().c_str()) ; //send request
	}
	delete[] dataBuff;
	if (send(sock, (char*)ssOrderID.str().c_str(), ssOrderID.str().length() + 1) == -1) { LOG("Error in send msg in Session\n"); return; }
	LOG("Session stopped sockID = " << sock << std::endl);
}

void timer(std::list<Session*> &sessions, bool &exitFlag)
{
	try {
		while (exitFlag)
			if (!sessions.empty())
				if (sessions.front()->IsTimeUp())
				{
					delete sessions.front();
					sessions.pop_front();
				}
	}
	catch (const std::exception& ex){
		LOG(ex.what());
		Session::Log.close();
		DB::stop();
		exit(-1);
	}
	LOG("End timer\n");
}

int server()
{
	DB::start();
	std::list<Session*> sessions;
	bool exitTimer = 1, exitServer = 1;
	std::thread Timer(timer, std::ref(sessions), std::ref(exitTimer));
	Session::Log.open("Log", std::ios::app);
	SOCKET server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == -1) DEATH("Socket failed!\n", 1);
	struct sockaddr_in addr = {0};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(5555);
	addr.sin_addr.s_addr = INADDR_ANY;
	if (bind(server, (struct sockaddr*) &addr, sizeof(addr)) == -1) { closesocket(server); DEATH("Bind failed!\n", 2); }
	if (listen(server, 256) == -1) { closesocket(server); DEATH("Listen failed!\n", 3); }
	LOG("Server started\n");
	while (1)
	{
		try {
			if (!exitServer) break;
			struct sockaddr_in clientAddr = { 0 };
			int structClientLen = sizeof(addr);
			SOCKET  newClient = accept(server, (struct sockaddr*)&clientAddr, &structClientLen);
			if (newClient == -1) { LOG("Accept failed!\n"); continue; }
			else {
				LOG("Session start with client on " 
					<< inet_ntoa(clientAddr.sin_addr) << ":" << clientAddr.sin_port
					<< ", sockID = " << newClient << std::endl);
				sessions.push_back(new Session(newClient));
			}
		}
		catch (const std::exception& ex) DEATH("Unexpected error in session processing. Ex msg: " << ex.what(), 4);
	}
	try {
		while (!sessions.empty())
		{
			delete sessions.front();
			sessions.pop_front();
		}
	}
	catch (const std::exception& ex) DEATH("Unexpected error in sessions destruct. Ex msg: " << ex.what(), 5);
	exitTimer = 0;
	Timer.join();
	DEATH("Server exit\n", 0);
}
