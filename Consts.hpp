#ifndef Consts_hpp

#define Consts_hpp
#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <random>
#include <algorithm>
#include <vector>
#include <sstream>
#include <string>

/*		網路定義	*/
#define BUF_LENGH 512
#define SERVER_ADDRESS "127.0.0.1"
const uint16_t PORT = 7171;
SOCKET sockfd;					/* 紀錄 socket 代號		*/

/*		伺服端無除錯用	*/
#define DEBUG_HEAD			"[DEBUG]"
#define INFO_HEAD				"[INFO]"
#define WARNING_HEAD		"[WARNING]"

#define WAIT_CHOSE_DELAY 500	//等待所有玩家選擇 20000

/*		函式宣告	*/
std::vector<std::string> commandParser(const std::string);
void commandHandle(const std::string, const struct sockaddr_in);
void doBroadcast(std::string);
std::string Int_to_String(int);
void doBroadcastPlayerList();

struct CLIENT
{
	char _ip[20];
	struct sockaddr_in _sock;		/*		玩家位置					*/
	std::string _name;				/*		玩家名稱					*/
	bool _connect;					/*		玩家連線狀態				*/
	int score = 0;						/*		玩家分數					*/
	bool state = true;				/*		紀錄本回合是否探險	*/
	CLIENT() {}
	CLIENT(struct sockaddr_in sock) :_sock(sock) {}
	void setup(char* ip, struct sockaddr_in sock, std::string name) {
		//strcpy_(_ip, ip);
		_sock = sock;
		_name = name;
		_connect = true;
	}
	/*		重置玩家選擇		*/
	void restart() {
		state = true;
	}
	/*		玩家分數增加		*/
	void addScore(int s) {
		score += s;
	}
	/*		傳送封包給本玩家	*/
	void sendPacket(std::string msg) {
		if (_connect == true && sendto(sockfd, msg.c_str(), BUF_LENGH, 0, (struct sockaddr*)&_sock, sizeof(_sock)) <= 0) {
			_connect = false;
			perror(("[Error]: " + _name + "sendto() failed.").c_str());
			return;
		}
	}
};

std::vector<CLIENT> users;

/*		TRUE/FALSE	*/
enum Chose {
	F = 0,
	T = 1
};

#define WAIT_DELAY 500	//等待封包間隔

/*		操作碼 傳給客戶端封包用	*/
#define GETCHOSE				"getchose;"
#define INFO						"info;"
#define ROUND					"round;"
#define CARD						"card;"
#define GAME_END			"end;"
#define PLAYERLIST			"playerlist;"
#define PLAYERLISTFULL	"playerlistfull;"

/*		操作碼 辨識客戶端封包用 沒有';'	*/
#define NAME					"name"
#define CHOSE					"chose"

uint8_t JoinedPlayer = 0;	/*		用於網路確認當前人數	*/
/*
 * 將 Int 類型 轉換為 String 類型
 * @param n 轉換前的 Int 資料
 * @return 緩換成 String 的資料
 */
std::string Int_to_String(int n)
{
	std::ostringstream stream;
	stream << n;
	return stream.str();
}
struct LOG
{
	struct sockaddr_in sock;
	bool state;
};
std::vector<LOG> logs;
/*
 * 辨識指令並做出相對應處理
 * @param cmd 要辨識的字串指令
 */
void commandHandle(const std::string cmd, const struct sockaddr_in sock) {
	std::vector<std::string> v_cmd = commandParser(cmd);	//
	int size = v_cmd.size();
	if (size < 1) { return; }	//DROP
	//玩家加入房間
	if (size == 2 && v_cmd[0] == NAME) {
		CLIENT nClient; nClient.setup((char*)"", sock, v_cmd[1]);
		users.push_back(nClient);
		std::cerr << "新玩家加入!!" << std::endl;
		doBroadcastPlayerList();
		return;
	}
	//等待玩家狀態
	if (size == 2 && v_cmd[0] == CHOSE) {
		std::vector<CLIENT>::iterator it;
		LOG log;
		log.sock = sock;
		if (v_cmd[1] == "0")
			log.state = false;
		else
			log.state = true;
		std::cerr << "有玩家選擇了 " << log.state << std::endl;
		logs.push_back(log);
	}
	return;
}
/*
 * 將玩家清單廣播給每位玩家
 */
void sendPlayerList() {
	std::string list = "";
	int lengh = users.size();
	for (int it = 0; it <= lengh; it++) {
		list += users.at(0)._name;
		if (it != lengh)
			list += ";";
	}
	doBroadcast(PLAYERLIST + list);
}
/*
 * 解析指令
 * @param command 要分割的字串
 * @return 分割後的字串所組成的 vector
 */
std::vector<std::string> commandParser(const std::string command) {
	const char delim = ';';					//分割條件
	std::stringstream ss(command);
	std::vector<std::string> cv;
	std::string str;
	while (std::getline(ss, str, delim)) {
		cv.emplace_back(std::move(str));
	}
	return cv;		//返回結果
}
/*
 * 對單一玩家傳送封包
 * @param client 傳送目的端
 * @param msg 要傳送的訊息
 */
void doSendPacket(CLIENT client, std::string msg) {
	client.sendPacket(msg);
}
/*
 * 對所有玩家進行廣播
 * @param msg 要傳送的訊息
 */
void doBroadcast(std::string msg) {
	for (auto it = users.begin(); it != users.end(); it++)
	{
		(*it).sendPacket(msg);
	}
}
/*
 * 對所有玩家廣播玩家清單
 */
void doBroadcastPlayerList() {
	std::string msg = "";
	int size = users.size();
	for (int i = 0; i < size; i++)
	{
		msg += users[i]._name;
		if (i != size)
			msg += ";";
	}
	doBroadcast(PLAYERLIST + msg);
}
/*
	錯誤處理
	@param err_msg 錯誤訊息.
*/
void perror(char* err_msg) {
	std::cout << err_msg << " 錯誤代碼: " << WSAGetLastError() << std::endl;
}

#endif // !Consts_hpp