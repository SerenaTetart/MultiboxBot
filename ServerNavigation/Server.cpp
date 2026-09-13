#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdio>
#include <cstdint>
#include <thread>
#include "Navigation.h"

#include <sstream>
#include <iomanip>
#include <iostream>
#include <cstring>

#pragma comment(lib, "Ws2_32.lib")

#include <mutex>
static std::mutex g_compute_mutex;

static bool parse_two_vec3_and_map(const std::string& s, Vector3& a, Vector3& b, int& mapID) {
    float ax, ay, az, bx, by, bz;
    int mid;
    int n = std::sscanf(s.c_str(),
        " %f , %f , %f  %f , %f , %f  %d",
        &ax, &ay, &az, &bx, &by, &bz, &mid);
    if (n != 7) return false;
    a = Vector3{ ax, ay, az };
    //std::cout << "a: " << a.X << ", " << a.Y << ", " << a.Z << "\n";
    b = Vector3{ bx, by, bz };
    //std::cout << "b: " << b.X << ", " << b.Y << ", " << b.Z << "\n";
    mapID = mid;
    //std::cout << "mapID: " << mapID << "\n";
    return true;
}

static bool parse_add_blacklist(const std::string& payload, unsigned int& mapID, std::string& name, Vector3& position, float& radius, unsigned int& type) {
    std::istringstream iss(payload);
    std::string positionStr;

    if (!(iss >> mapID >> name >> positionStr >> radius >> type)) return false;

    float x, y, z;
    int n = std::sscanf(positionStr.c_str(), "%f,%f,%f", &x, &y, &z);
    if (n != 3) return false;

    position = Vector3{x, y, z};

    return true;
}

static bool parse_remove_blacklist(const std::string& payload, std::string& name) {
    std::istringstream iss(payload);
    return static_cast<bool>(iss >> name);
}

static float trunc4(float v) {
    return std::trunc(v * 10000.0f) / 10000.0f;
}

static void handle_client(SOCKET csock, sockaddr_in caddr) {
    char ip[INET_ADDRSTRLEN]{};
    InetNtopA(AF_INET, &caddr.sin_addr, ip, sizeof(ip));
    std::printf("ServerNavigation: [+] Client %s:%u connected\n", ip, ntohs(caddr.sin_port));

    while (true) {
        char buffer[128];
        int r = recv(csock, buffer, sizeof(buffer), 0);

        if (r == 0) break;
        if (r == SOCKET_ERROR) {
            int err = WSAGetLastError();
            if (err == WSAETIMEDOUT) continue;
            break;
        }
        if (r < 2) continue;

        if (buffer[0] == 'C') {
            std::string payload(buffer + 1, buffer + r);

            Vector3 start_pos{}, end_pos{}, result{};
            int mapID;

            if (!parse_two_vec3_and_map(payload, start_pos, end_pos, mapID)) {
                result = start_pos;
            }
            else {
                std::lock_guard<std::mutex> lock(g_compute_mutex);

                result = Navigation::ComputePath(mapID, start_pos, end_pos);
            }

            std::ostringstream oss;

            oss << std::fixed
                << std::setprecision(4)
                << trunc4(result.X) << ","
                << trunc4(result.Y) << ","
                << trunc4(result.Z);

            auto out = oss.str();

            send(csock, out.c_str(), static_cast<int>(out.size()), 0);
        }
        else if (buffer[0] == 'B') {
            std::string payload(buffer + 1, buffer + r);

            unsigned int mapID = 0;
            unsigned int type = 0;

            std::string name;
            Vector3 position{};

            float radius = 0.0f;

            if (!parse_add_blacklist(payload, mapID, name, position, radius, type)) {
                const char* response = "ERROR";
                send(csock, response, static_cast<int>(std::strlen(response)), 0);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(g_compute_mutex);
                Navigation::AddBlackList(mapID, name, position, radius, type);
            }

            const char* response = "OK";

            send(csock, response, static_cast<int>(std::strlen(response)), 0);
        }
        else if (buffer[0] == 'R') {
            std::string payload(buffer + 1, buffer + r);
            std::string name;

            if (!parse_remove_blacklist(payload, name)) {
                const char* response = "ERROR";
                send(csock, response, static_cast<int>(std::strlen(response)), 0);
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(g_compute_mutex);
                Navigation::RemoveFromBlacklist(name);
            }

            const char* response = "OK";
            send(csock, response, static_cast<int>(std::strlen(response)), 0);
        }
        else continue;
    }

    ::closesocket(csock);
    std::printf("ServerNavigation: [-] Client %s:%u disconnected\n", ip, ntohs(caddr.sin_port));
}

struct WarmupPath {
    unsigned int mapId;
    Vector3 start;
    Vector3 end;
};

static const WarmupPath warmups[] = {
    { 0, { -11105.501953f, 1486.848267f, 32.607071f }, { -11207.504883f, 1680.481934f, 24.358500f } }
};

static void WarmupNavigation() {
    std::lock_guard<std::mutex> lock(g_compute_mutex);

    for (const auto& w : warmups) {
        Vector3 r = Navigation::ComputePath(w.mapId, w.start, w.end);
        std::cout << "Warmup map " << w.mapId
            << " -> " << r.X << "," << r.Y << "," << r.Z << "\n";
    }
}

int main() {
    Navigation::Load("Navigation.dll");

    WarmupNavigation();

    WSADATA wsa{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::printf("ServerNavigation: WSAStartup failed\n");
        return 1;
    }

    SOCKET lsock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (lsock == INVALID_SOCKET) {
        std::printf("ServerNavigation: socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    BOOL yes = TRUE;
    setsockopt(lsock, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof(yes));

    sockaddr_in sin{};
    sin.sin_family = AF_INET;
    sin.sin_port = htons(50002);
    InetPtonA(AF_INET, "127.0.0.1", &sin.sin_addr);

    if (bind(lsock, (sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        std::printf("ServerNavigation: bind() failed: %d\n", WSAGetLastError());
        closesocket(lsock); WSACleanup(); return 1;
    }
    if (listen(lsock, SOMAXCONN) == SOCKET_ERROR) {
        std::printf("ServerNavigation: listen() failed: %d\n", WSAGetLastError());
        closesocket(lsock); WSACleanup(); return 1;
    }

    std::printf("ServerNavigation: listening on 127.0.0.1:50002\n");
    while (true) {
        sockaddr_in caddr{}; int clen = sizeof(caddr);
        SOCKET csock = accept(lsock, (sockaddr*)&caddr, &clen);
        if (csock == INVALID_SOCKET) {
            std::printf("ServerNavigation: accept() failed: %d\n", WSAGetLastError());
            continue;
        }
        std::thread(handle_client, csock, caddr).detach();
    }
}