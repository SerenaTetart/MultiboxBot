#include <iostream>
#include <string>

#include "Client.h"
#include "Game.h"
#include "Functions.h"

#pragma comment(lib, "ws2_32.lib")

bool Client::bot_running;
bool Client::client_running;
SOCKET Client::sock;
WSADATA Client::WSAData;

char* subchar(char* txt, int begin, int end) {
	char* res = new char[end-begin];
	for (int i = 0; i < end-begin; i++) {
		res[i] = *(txt + begin + i);
	}
	res[end-begin] = '\0';
	return res;
}

void Client::sendMessage(std::string msg) {
	//Send message
	send(sock, msg.c_str(), msg.length(), 0);
}

void Client::recvMessage() {
	char buffer[128];
	recv(sock, buffer, sizeof(buffer), 0);
	if (buffer[0] == 'B') {
		char* buffTmp = subchar(buffer, 5, 7);
		if (strcmp(buffTmp, "ON") == 0) {
			std::cout << "ON activated" << "\n";
			bot_running = true;
		} else if(strcmp(buffTmp, "OFF")) {
			std::cout << "OFF activated" << "\n";
			bot_running = false;
		}
		std::cout << "Bot running: " << bot_running << "\n";
	}
	else if (buffer[0] == 'S') {
		char* buffTmp = subchar(buffer, 6, 7);
		playerSpec = ((int)buffTmp[0] - '0');
	}
	else if (buffer[0] == 'C' && buffer[1] == 'r' && buffer[2] == 'a' && buffer[3] == 'f' && buffer[4] == 't') {
		unsigned int index = buffer[5] - '0';
		unsigned int id = (buffer[6] - '0') * 10 + (buffer[7] - '0');
		unsigned int str_length = (buffer[8] - '0') * 10 + (buffer[9] - '0');
		unsigned int skill = buffer[10] - '0';
		std::string playerName = subchar(buffer, 11, 11 + str_length);

		if (leaderInfos.size() < id + 1) {
			while (leaderInfos.size() < id) {
				leaderInfos.push_back(std::tuple<std::string, int, int, int>("tmp", -1, 0, 0));
			}
			if(index == 0) leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, -1, skill, 0));
			else leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, -1, 0, skill));
		}
		else if (index == 0) leaderInfos[id] = std::tuple<std::string, int, int, int>(playerName, get<1>(leaderInfos[id]), skill, get<3>(leaderInfos[id]));
		else leaderInfos[id] = std::tuple<std::string, int, int, int>(playerName, get<1>(leaderInfos[id]), get<2>(leaderInfos[id]), skill);
	}
	else if (buffer[0] == 'R' && buffer[1] == 'o' && buffer[2] == 'l' && buffer[3] == 'e') {
		int role = buffer[4] - '0';
		unsigned int id = (buffer[6] - '0')*10 + (buffer[7] - '0');
		int str_length = (buffer[9] - '0') * 10 + (buffer[10] - '0');
		std::string playerName = subchar(buffer, 12, 12+str_length);
		//std::cout << "playername: " << playerName << " role: " << role << "\n";

		if (leaderInfos.size() < id+1) {
			while (leaderInfos.size() < id) {
				leaderInfos.push_back(std::tuple<std::string, int, int, int>("tmp", -1, 0, 0));
			}
			leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, role, 0, 0));
		}
		else leaderInfos[id] = std::tuple<std::string, int, int, int>(playerName, role, get<2>(leaderInfos[id]), get<3>(leaderInfos[id]));
	}
	else if (buffer[0] == 'K' && buffer[1] == '1') {
		keybindTrigger = 1;
	}
	else if (buffer[0] == 'K' && buffer[1] == '2') {
		keybindTrigger = 2;
	}
	else if (buffer[0] == 'K' && buffer[1] == '3') {
	        autoLearnSpells = (autoLearnSpells + 1) % 4;
	}
	else if (buffer[0] == 'K' && buffer[1] == '4') {
		if (passiveGroup) passiveGroup = false;
		else passiveGroup = true;
	}
	else if (buffer[0] == 'C' && buffer[1] == '1') {
		//Main Character no automation
		MCNoAuto = ((int)buffer[2] - '0');
	}
	else if (buffer[0] == 'C' && buffer[1] == '2') {
		//Main Character auto move
		MCAutoMove = ((int)buffer[2] - '0');
	}
	else if (buffer[0] == 'Q' && buffer[1] == 'U' && buffer[2] == 'I' && buffer[3] == 'T') {
		client_running = false;
		std::cout << "QUITTING\n";
	}
}

bool Client::ConnectToServer(const char* addr, int port) {
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return false;
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sock = socket(AF_INET, SOCK_STREAM, 0);

	std::cout << "Connecting to Navigation server...\n";
	if (connect(sock, (SOCKADDR*)&sin, sizeof(sin)) == SOCKET_ERROR) {
		closesocket(sock);
		WSACleanup();
		return false;
	}
	std::cout << "Connected to " << addr << ":" << port << "\n";
	return true;
}

void Client::DisconnectClient() {
	closesocket(sock);
	WSACleanup();
	std::cout << "Client disconnected !\n";
}

DWORD WINAPI Client::MakeLoop(void* data) {
	//Read messages from server on a separate thread then do something
	ConnectToServer("127.0.0.1", 50001);
	while (client_running == true) {
		recvMessage();
	}
	DisconnectClient();
	return 0;
}
