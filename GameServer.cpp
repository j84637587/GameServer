#include <iostream>
#include <winsock2.h>
#include <WS2tcpip.h>
#include <process.h>
#include "Consts.hpp"
#include "GameServer.hpp"

/*
	接收每個客戶端連線的處理函式 (多執行緒)
	@param *param The binded socket ID.
*/
void recv_thread(void *param) {
	int recv_len;
	SOCKET sock_id = (SOCKET)param;
	struct sockaddr_in local;
	int sadlen = sizeof(local);
	std::vector<std::string> cmd;	/*	拿來儲存指令	*/
	char buf[BUF_LENGH];

	/*	Recive Loop		*/
	for (;;) {
		memset(buf, 0, BUF_LENGH);
		recv_len = recvfrom(sock_id, buf, BUF_LENGH, 0, (struct sockaddr*)&local, &sadlen);
		if (recv_len <= 0) {
			perror("recvform() failed.");
			return;
		}
		printf("接收到 -> %s \n", buf);

		//指令處理
		commandHandle(buf, local);
	}
}

/*		程式進入點	*/
int main(int argc, char **argv) {
	/* Socket Variables  */
	struct sockaddr_in server;	/*	伺服器位址			*/
	//struct sockaddr_in client;		/*	客戶端位址(暫存)	*/
	int length = sizeof(server);	/*	sockaddr_in長 度	*/
	//int ret;								/*	暫存紀錄				*/
	//SOCKET sockfd;				/* 紀錄 socket 代號	(改成全域)	*/
	WSADATA wsa;					/*	網路環境				*/


	/*		Socket environment setup	*/
	std::cerr << "初始化socket環境中...  ";
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		perror("初始化 Socket 失敗. ");
		return -1;
	}
	std::cerr << "[完成]" << std::endl;

	/*	建立 socket */
	std::cerr << "建立 socket 中...  ";
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == INVALID_SOCKET) {
		perror("建立 Socket 失敗. ");
		exit(EXIT_FAILURE);
	}
	std::cerr << "[完成]" << std::endl;	//sock -> socket id

	/*	設定伺服器位址 */
	std::cerr << "設定 socket 資料中...  ";
	memset(&server, 0, length);					//重置socket
	server.sin_family = AF_INET;					//IPv4
	server.sin_addr.s_addr = INADDR_ANY;	//所有網卡
	server.sin_port = htons(PORT);				//port 預設 : 7171
	std::cerr << "[完成]" << std::endl;
	/* 綁定本地 IP/Port */
	if (bind(sockfd, (struct sockaddr *)&server, length) != 0){
		perror("綁定 Socket 失敗. ");
		exit(EXIT_FAILURE);
	}
	/*		持續監聽Port		*/
	_beginthread(recv_thread, 0, (void*)sockfd);
	std::cout << "服務端已經啟動，正在監聽 Port : " << PORT << std::endl;

	//等待玩家人數 (無強制開始)
	while (users.size() < 3) {
		Sleep(WAIT_DELAY);
	}

	/*		開始遊戲內容		*/
	Room room(users.size()); //建立房間 並初始化 房間人數 最大回合數 分數 使用 users.size() 只是為了安全
	while (!room.isGameEnd()) {
		std::cerr << INFO_HEAD << " 回合 #" << room.currentRound << " 開始!\n";
		room.doBroadcastInfo();		//場上卡牌
		doBroadcast(ROUND + Int_to_String(room.currentRound));
		//判斷回合是否結束, -1為遊戲繼續
		while (room.isRoundEnd() == -1)
		{
			room.doBroadcastPlayerInfo();	//玩家資訊
			room.draw();		//抽牌
			if (room.drawedCards.back().cardType == CardType::Monster)
			{
				std::cout << "翻開了卡片!!  卡片類型: 怪物卡   種類: " << room.drawedCards.back().score << std::endl;
			}
			else if (room.drawedCards.back().cardType == CardType::Treasure)
			{
				std::cout << "翻開了卡片!!  卡片類型: 寶藏卡   分數: " << room.drawedCards.back().score << std::endl;
			}
			else {
				std::cout << "翻開了卡片!!  卡片類型: 神器卡  分數: " << room.drawedCards.back().score << std::endl;
			}
			room.doBroadcastCardInfo();
			//判斷是否結束, 結束的話就進行相對處裡
			if (room.isRoundEnd() != -1) { break; }
			// 取得每位冒險玩家的操作
			logs.clear();
			//詢問玩家是否繼續
			for (auto it = room.currentAdvanturePlayer.begin(); it != room.currentAdvanturePlayer.end(); it++)
			{
				doSendPacket(*it, GETCHOSE);
			}
			//等待所有玩家回應 (沒回應的都為 回家) 20s
			for (int i = 0; i < 40; i++) 
			{
				Sleep(WAIT_CHOSE_DELAY);
				if (logs.size() == room.getAdventureNumber())	//所有玩家都選擇了
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
			/*		對玩家所做的選擇做調整		*/
			for (auto it = room.currentAdvanturePlayer.begin(); it != room.currentAdvanturePlayer.end(); it++)
			{
				if ((*it).state == false) {
					room.currentBackPlayer.insert(room.currentBackPlayer.end(), *it);
					it = room.currentAdvanturePlayer.erase(it);
				}
			}
			std::cout << "此次回家人數: " << room.getBackNumber() << "     繼續冒險人數: " << room.getAdventureNumber() << " \n";
			// 一人回家 - 獲得神器卡, 並獨得通道上寶石
			if (room.getBackNumber() == 1) {
				if (room.currentBackPlayer.back().state == true)	//錯誤判斷
					throw "Error: 該回家的玩家沒回家, 看來他流連忘返";
				room.doAllocationScore();									//分配分數給回家的玩家並更新場上分數
				room.doMoveBackToReturn();								//將回家的玩家移到已回家
			}
			//多人回家 - 神器卡留在通道上, 平分寶石
			else if (room.getBackNumber() > 1) {
				room.doAllocationScore();									//分配分數給回家的玩家並更新場上分數
				room.doMoveBackToReturn();								//將回家的玩家移到已回家
			}
			//沒人回家 - 交給while處理
			else {
				continue;
			}
		}
		room.doRestart();							//重置玩家狀態(探險), 回合分數為0
		room.currentRound++;
	}
	room.finalResults();

	closesocket(sockfd);
	WSACleanup();
	return 0;
}


/*
 * 除錯訊息
 */
void Debugmsg(std::string p, int score, int pcur) {
	Debugmsg(p, score, pcur, 0);
}
/*
 * 除錯訊息
 */
void Debugmsg(std::string p, int score, int pcur, int ascore) {
	std::cout << "玩家 " << p
		<< "	 獲得寶石分數: " << score
		<< "  目前總獲得分數: " << pcur;
	if (ascore)
		std::cout << "  獲得神器卡 分數: " << ascore;
	std::cout << std::endl;
}