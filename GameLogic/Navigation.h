#pragma once

#include <windows.h>

#include "WoWObject.h"

class Navigation {
public:
	static bool ConnectToServer(const char* addr, int port);
    static Position CalculatePath(unsigned int mapId, const Position start, const Position end);
	static void DisconnectClient();
private:
	static bool is_socket_connected(SOCKET s);
	static WSADATA WSAData;
	static SOCKET sock;
};