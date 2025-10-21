#pragma once
#include <string>

#include <sys/types.h>
#include <winsock2.h>

class Client {
	public:
		//Attributes
		static bool bot_running;
		static bool client_running;
		//Methods
		static bool ConnectToServer(const char* addr, int port);
		static void sendMessage(std::string msg);
		static void recvMessage();
		static void DisconnectClient();
		static DWORD WINAPI MakeLoop(void* data);
	private:
		static WSADATA WSAData;
		static SOCKET sock;
};