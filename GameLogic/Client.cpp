#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <cstring>
#include <winsock2.h>
#include <windows.h>

#include "Client.h"
#include "Game.h"
#include "Functions.h"

#pragma comment(lib, "ws2_32.lib")

bool Client::bot_running = false;
bool Client::client_running = true;
SOCKET Client::sock = INVALID_SOCKET;
WSADATA Client::WSAData;

static std::string subStringSafe(const char* txt, int begin, int end) {
    if (txt == nullptr || begin < 0 || end < begin) {
        return "";
    }
    return std::string(txt + begin, txt + end);
}

void Client::sendMessage(std::string msg) {
    if (sock == INVALID_SOCKET) {
        return;
    }
    send(sock, msg.c_str(), static_cast<int>(msg.length()), 0);
}

void Client::recvMessage() {
    char buffer[256] = { 0 };

    int len = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (len <= 0) {
        client_running = false;
        return;
    }

    buffer[len] = '\0';

    if (buffer[0] == 'B') {
        if (std::strncmp(buffer, "Bot: ON", 7) == 0) {
            std::cout << "ON activated\n";
            bot_running = true;
        }
        else if (std::strncmp(buffer, "Bot: OFF", 8) == 0) {
            std::cout << "OFF activated\n";
            bot_running = false;
        }

        std::cout << "Bot running: " << bot_running << "\n";
    }
    else if (buffer[0] == 'S') {
        // "Spec: X "
        if (len >= 7) {
            playerSpec = static_cast<int>(buffer[6] - '0');
        }
    }
    else if (std::strncmp(buffer, "Craft", 5) == 0) {
        if (len < 11) {
            return;
        }

        unsigned int index = static_cast<unsigned int>(buffer[5] - '0');
        unsigned int id = static_cast<unsigned int>((buffer[6] - '0') * 10 + (buffer[7] - '0'));
        unsigned int str_length = static_cast<unsigned int>((buffer[8] - '0') * 10 + (buffer[9] - '0'));
        unsigned int skill = static_cast<unsigned int>(buffer[10] - '0');

        if (11 + static_cast<int>(str_length) > len) {
            return;
        }

        std::string playerName = subStringSafe(buffer, 11, 11 + static_cast<int>(str_length));

        if (leaderInfos.size() < id + 1) {
            while (leaderInfos.size() < id) {
                leaderInfos.push_back(std::tuple<std::string, int, int, int>("tmp", -1, 0, 0));
            }

            if (index == 0) {
                leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, -1, skill, 0));
            }
            else {
                leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, -1, 0, skill));
            }
        }
        else {
            if (index == 0) {
                leaderInfos[id] = std::tuple<std::string, int, int, int>(
                    playerName,
                    std::get<1>(leaderInfos[id]),
                    skill,
                    std::get<3>(leaderInfos[id])
                );
            }
            else {
                leaderInfos[id] = std::tuple<std::string, int, int, int>(
                    playerName,
                    std::get<1>(leaderInfos[id]),
                    std::get<2>(leaderInfos[id]),
                    skill
                );
            }
        }
    }
    else if (std::strncmp(buffer, "Role", 4) == 0) {
        // Format: Role{role}_{id}_{len}_{name}
        if (len < 12) {
            return;
        }

        int role = buffer[4] - '0';
        unsigned int id = static_cast<unsigned int>((buffer[6] - '0') * 10 + (buffer[7] - '0'));
        int str_length = (buffer[9] - '0') * 10 + (buffer[10] - '0');

        if (12 + str_length > len) {
            return;
        }

        std::string playerName = subStringSafe(buffer, 12, 12 + str_length);

        if (leaderInfos.size() < id + 1) {
            while (leaderInfos.size() < id) {
                leaderInfos.push_back(std::tuple<std::string, int, int, int>("tmp", -1, 0, 0));
            }
            leaderInfos.push_back(std::tuple<std::string, int, int, int>(playerName, role, 0, 0));
        }
        else {
            leaderInfos[id] = std::tuple<std::string, int, int, int>(
                playerName,
                role,
                std::get<2>(leaderInfos[id]),
                std::get<3>(leaderInfos[id])
            );
        }
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
        passiveGroup = !passiveGroup;
    }
    else if (buffer[0] == 'C' && buffer[1] == '1') {
        // Main Character no automation
        MCNoAuto = buffer[2] - '0';
        if (MCNoAuto == 1) {
            MCAutoMove = 0;
        }
    }
    else if (buffer[0] == 'C' && buffer[1] == '2') {
        // Main Character auto move
        MCAutoMove = buffer[2] - '0';
        if (MCAutoMove == 1) {
            MCNoAuto = 0;
        }
    }
    else if (std::strncmp(buffer, "QUIT", 4) == 0) {
        client_running = false;
        std::cout << "QUITTING\n";
    }
}

bool Client::ConnectToServer(const char* addr, int port) {
    if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
        return false;
    }

    SOCKADDR_IN sin{};
    sin.sin_addr.s_addr = inet_addr(addr);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(static_cast<u_short>(port));

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        WSACleanup();
        return false;
    }

    std::cout << "Connecting to Navigation server...\n";

    if (connect(sock, reinterpret_cast<SOCKADDR*>(&sin), sizeof(sin)) == SOCKET_ERROR) {
        closesocket(sock);
        sock = INVALID_SOCKET;
        WSACleanup();
        return false;
    }

    std::cout << "Connected to " << addr << ":" << port << "\n";
    return true;
}

void Client::DisconnectClient() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    WSACleanup();
    std::cout << "Client disconnected !\n";
}

DWORD WINAPI Client::MakeLoop(void* data) {
    if (!ConnectToServer("127.0.0.1", 50001)) {
        std::cout << "Unable to connect to server.\n";
        return 0;
    }

    while (client_running == true) {
        recvMessage();
    }

    DisconnectClient();
    return 0;
}