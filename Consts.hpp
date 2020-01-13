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

/*		�����w�q	*/
#define BUF_LENGH 512
#define SERVER_ADDRESS "127.0.0.1"
const uint16_t PORT = 7171;
SOCKET sockfd;					/* ���� socket �N��		*/

/*		���A�ݵL������	*/
#define DEBUG_HEAD			"[DEBUG]"
#define INFO_HEAD				"[INFO]"
#define WARNING_HEAD		"[WARNING]"

#define WAIT_CHOSE_DELAY 500	//���ݩҦ����a��� 20000

/*		�禡�ŧi	*/
std::vector<std::string> commandParser(const std::string);
void commandHandle(const std::string, const struct sockaddr_in);
void doBroadcast(std::string);
std::string Int_to_String(int);
void doBroadcastPlayerList();

struct CLIENT
{
	char _ip[20];
	struct sockaddr_in _sock;		/*		���a��m					*/
	std::string _name;				/*		���a�W��					*/
	bool _connect;					/*		���a�s�u���A				*/
	int score = 0;						/*		���a����					*/
	bool state = true;				/*		�������^�X�O�_���I	*/
	CLIENT() {}
	CLIENT(struct sockaddr_in sock) :_sock(sock) {}
	void setup(char* ip, struct sockaddr_in sock, std::string name) {
		//strcpy_(_ip, ip);
		_sock = sock;
		_name = name;
		_connect = true;
	}
	/*		���m���a���		*/
	void restart() {
		state = true;
	}
	/*		���a���ƼW�[		*/
	void addScore(int s) {
		score += s;
	}
	/*		�ǰe�ʥ]�������a	*/
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

#define WAIT_DELAY 500	//���ݫʥ]���j

/*		�ާ@�X �ǵ��Ȥ�ݫʥ]��	*/
#define GETCHOSE				"getchose;"
#define INFO						"info;"
#define ROUND					"round;"
#define CARD						"card;"
#define GAME_END			"end;"
#define PLAYERLIST			"playerlist;"
#define PLAYERLISTFULL	"playerlistfull;"

/*		�ާ@�X ���ѫȤ�ݫʥ]�� �S��';'	*/
#define NAME					"name"
#define CHOSE					"chose"

uint8_t JoinedPlayer = 0;	/*		�Ω�����T�{��e�H��	*/
/*
 * �N Int ���� �ഫ�� String ����
 * @param n �ഫ�e�� Int ���
 * @return �w���� String �����
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
 * ���ѫ��O�ð��X�۹����B�z
 * @param cmd �n���Ѫ��r����O
 */
void commandHandle(const std::string cmd, const struct sockaddr_in sock) {
	std::vector<std::string> v_cmd = commandParser(cmd);	//
	int size = v_cmd.size();
	if (size < 1) { return; }	//DROP
	//���a�[�J�ж�
	if (size == 2 && v_cmd[0] == NAME) {
		CLIENT nClient; nClient.setup((char*)"", sock, v_cmd[1]);
		users.push_back(nClient);
		std::cerr << "�s���a�[�J!!" << std::endl;
		doBroadcastPlayerList();
		return;
	}
	//���ݪ��a���A
	if (size == 2 && v_cmd[0] == CHOSE) {
		std::vector<CLIENT>::iterator it;
		LOG log;
		log.sock = sock;
		if (v_cmd[1] == "0")
			log.state = false;
		else
			log.state = true;
		std::cerr << "�����a��ܤF " << log.state << std::endl;
		logs.push_back(log);
	}
	return;
}
/*
 * �N���a�M��s�����C�쪱�a
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
 * �ѪR���O
 * @param command �n���Ϊ��r��
 * @return ���Ϋ᪺�r��Ҳզ��� vector
 */
std::vector<std::string> commandParser(const std::string command) {
	const char delim = ';';					//���α���
	std::stringstream ss(command);
	std::vector<std::string> cv;
	std::string str;
	while (std::getline(ss, str, delim)) {
		cv.emplace_back(std::move(str));
	}
	return cv;		//��^���G
}
/*
 * ���@���a�ǰe�ʥ]
 * @param client �ǰe�ت���
 * @param msg �n�ǰe���T��
 */
void doSendPacket(CLIENT client, std::string msg) {
	client.sendPacket(msg);
}
/*
 * ��Ҧ����a�i��s��
 * @param msg �n�ǰe���T��
 */
void doBroadcast(std::string msg) {
	for (auto it = users.begin(); it != users.end(); it++)
	{
		(*it).sendPacket(msg);
	}
}
/*
 * ��Ҧ����a�s�����a�M��
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
	���~�B�z
	@param err_msg ���~�T��.
*/
void perror(char* err_msg) {
	std::cout << err_msg << " ���~�N�X: " << WSAGetLastError() << std::endl;
}

#endif // !Consts_hpp