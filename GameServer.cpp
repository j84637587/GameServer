#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include "Consts.hpp"
#include "GameServer.hpp"

/*
	�����C�ӫȤ�ݳs�u���B�z�禡 (�h�����)
	@param *param The binded socket ID.
*/
void recv_thread(void *param) {
	int recv_len;
	SOCKET sock_id = (SOCKET)param;
	struct sockaddr_in local;
	int sadlen = sizeof(local);
	std::vector<std::string> cmd;	/*	�����x�s���O	*/
	char buf[BUF_LENGH];

	/*	Recive Loop		*/
	for (;;) {
		memset(buf, 0, BUF_LENGH);
		recv_len = recvfrom(sock_id, buf, BUF_LENGH, 0, (struct sockaddr*)&local, &sadlen);
		if (recv_len <= 0) {
			perror("recvform() failed.");
			return;
		}
		printf("������ -> %s \n", buf);

		//���O�B�z
		commandHandle(buf, local);
	}
}

/*		�{���i�J�I	*/
int main(int argc, char **argv) {
	/* Socket Variables  */
	struct sockaddr_in server;	/*	���A����}			*/
	//struct sockaddr_in client;		/*	�Ȥ�ݦ�}(�Ȧs)	*/
	int length = sizeof(server);	/*	sockaddr_in�� ��	*/
	//int ret;								/*	�Ȧs����				*/
	//SOCKET sockfd;				/* ���� socket �N��	(�令����)	*/
	WSADATA wsa;					/*	��������				*/


	/*		Socket environment setup	*/
	std::cerr << "��l��socket���Ҥ�...  ";
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		perror("��l�� Socket ����. ");
		return -1;
	}
	std::cerr << "[����]" << std::endl;

	/*	�إ� socket */
	std::cerr << "�إ� socket ��...  ";
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		perror("�إ� Socket ����. ");
		exit(EXIT_FAILURE);
	}
	std::cerr << "[����]" << std::endl;	//sock -> socket id

	/*	�]�w���A����} */
	std::cerr << "�]�w socket ��Ƥ�...  ";
	memset(&server, 0, length);					//���msocket
	server.sin_family = AF_INET;					//IPv4
	server.sin_addr.s_addr = INADDR_ANY;	//�Ҧ����d
	server.sin_port = htons(PORT);				//port �w�] : 7171
	std::cerr << "[����]" << std::endl;
	/* �j�w���a IP/Port */
	if (bind(sockfd, (struct sockaddr *)&server, length) != 0){
		perror("�j�w Socket ����. ");
		exit(EXIT_FAILURE);
	}
	/*		�����ťPort		*/
	_beginthread(recv_thread, 0, (void*)sockfd);
	std::cout << "�A�Ⱥݤw�g�ҰʡA���b��ť Port : " << PORT << std::endl;

	//���ݪ��a�H�� (�L�j��}�l)
	while (users.size() < 3) {
		Sleep(WAIT_DELAY);
	}

	/*		�}�l�C�����e		*/
	Room room(users.size()); //�إߩж� �ê�l�� �ж��H�� �̤j�^�X�� ���� �ϥ� users.size() �u�O���F�w��
	while (!room.isGameEnd()) {
		std::cerr << INFO_HEAD << " �^�X #" << room.currentRound << " �}�l!\n";
		room.doBroadcastInfo();		//���W�d�P
		doBroadcast(ROUND + Int_to_String(room.currentRound));
		//�P�_�^�X�O�_����, -1���C���~��
		while (room.isRoundEnd() == -1)
		{
			room.doBroadcastPlayerInfo();	//���a��T
			room.draw();		//��P
			if (room.drawedCards.back().cardType == CardType::Monster)
			{
				std::cout << "½�}�F�d��!!  �d������: �Ǫ��d   ����: " << room.drawedCards.back().score << std::endl;
			}
			else if (room.drawedCards.back().cardType == CardType::Treasure)
			{
				std::cout << "½�}�F�d��!!  �d������: �_�åd   ����: " << room.drawedCards.back().score << std::endl;
			}
			else {
				std::cout << "½�}�F�d��!!  �d������: �����d  ����: " << room.drawedCards.back().score << std::endl;
			}
			room.doBroadcastCardInfo();
			//�P�_�O�_����, �������ܴN�i��۹�B��
			if (room.isRoundEnd() != -1) { break; }
			// ���o�C��_�I���a���ާ@
			logs.clear();
			//�߰ݪ��a�O�_�~��
			for (auto it = room.currentAdvanturePlayer.begin(); it != room.currentAdvanturePlayer.end(); it++)
			{
				doSendPacket(*it, GETCHOSE);
			}
			//���ݩҦ����a�^�� (�S�^�������� �^�a) 20s
			for (int i = 0; i < 40; i++) 
			{
				Sleep(WAIT_CHOSE_DELAY);
				if (logs.size() == room.getAdventureNumber())	//�Ҧ����a����ܤF
					break;
			}
			for (auto it = logs.begin(); it != logs.end(); it++)
			{
				try
				{
					(*room.findAdventureBySock((*it).sock)).state = (*it).state;
				}
				catch (const std::exception&)
				{
					throw "ERROR";
				}
			}
			/*		�缾�a�Ұ�����ܰ��վ�		*/
			for (auto it = room.currentAdvanturePlayer.begin(); it != room.currentAdvanturePlayer.end(); it++)
			{
				if ((*it).state == false) {
					room.currentBackPlayer.insert(room.currentBackPlayer.end(), *it);
					it = room.currentAdvanturePlayer.erase(it);
				}
			}
			std::cout << "�����^�a�H��: " << room.getBackNumber() << "     �~��_�I�H��: " << room.getAdventureNumber() << " \n";
			// �@�H�^�a - ��o�����d, �ÿW�o�q�D�W�_��
			if (room.getBackNumber() == 1) {
				if (room.currentBackPlayer.back().state == true)	//���~�P�_
					throw "Error: �Ӧ^�a�����a�S�^�a, �ݨӥL�y�s�Ѫ�";
				room.doAllocationScore();									//���t���Ƶ��^�a�����a�ç�s���W����
				room.doMoveBackToReturn();								//�N�^�a�����a����w�^�a
			}
			//�h�H�^�a - �����d�d�b�q�D�W, �����_��
			else if (room.getBackNumber() > 1) {
				room.doAllocationScore();									//���t���Ƶ��^�a�����a�ç�s���W����
				room.doMoveBackToReturn();								//�N�^�a�����a����w�^�a
			}
			//�S�H�^�a - �浹while�B�z
			else {
				continue;
			}
		}
		room.doRestart();							//���m���a���A(���I), �^�X���Ƭ�0
		room.currentRound++;
	}
	room.finalResults();

	closesocket(sockfd);
	WSACleanup();
	return 0;
}


/*
 * �����T��
 */
void Debugmsg(std::string p, int score, int pcur) {
	Debugmsg(p, score, pcur, 0);
}
/*
 * �����T��
 */
void Debugmsg(std::string p, int score, int pcur, int ascore) {
	std::cout << "���a " << p
		<< "	 ��o�_�ۤ���: " << score
		<< "  �ثe�`��o����: " << pcur;
	if (ascore)
		std::cout << "  ��o�����d ����: " << ascore;
	std::cout << std::endl;
}