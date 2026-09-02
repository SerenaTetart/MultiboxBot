#pragma once

#include <windows.h>

#include "WoWObject.h"

#include <vector>
#include <string>

struct BlacklistEntry
{
    std::string Name;
    Position Position;
    float Radius;
};

class Navigation {
public:
    static bool ConnectToServer(const char* addr, int port);

    static Position CalculatePath(
        unsigned int mapId,
        const Position start,
        const Position end
    );

    static bool AddBlacklist(
        unsigned int mapId,
        const std::string& name,
        const Position& position,
        float radius,
        unsigned int type = 55
    );

    static bool RemoveBlacklist(
        const std::string& name
    );

	static void ClearBlacklists();
    static bool HasBlacklists();
    static void DisconnectClient();
private:
	static bool IsBlacklisted(const std::string& name);
    static bool is_socket_connected(SOCKET s);

    static WSADATA WSAData;
    static SOCKET sock;
	static std::vector<BlacklistEntry> Blacklists;
};