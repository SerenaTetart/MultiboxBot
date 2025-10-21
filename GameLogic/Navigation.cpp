#include "Navigation.h"
#include <iostream>
#include <sstream>
#include <iomanip>

SOCKET Navigation::sock;
WSADATA Navigation::WSAData;

bool Navigation::is_socket_connected(SOCKET s) {
	if (s == INVALID_SOCKET) return false;
	sockaddr_in peer{}; int len = sizeof(peer);
	return ::getpeername(s, reinterpret_cast<sockaddr*>(&peer), &len) == 0;
}

static bool parse_vec3_cstr(const char* s, Position& p) {
	float x, y, z;
	int n = std::sscanf(s, " %f , %f , %f", &x, &y, &z);
	if (n != 3) return false;
	p.X = x; p.Y = y; p.Z = z;
	return true;
}

void appendPosition(std::string& msg, Position& pos) {
	// truncate at 4 decimals
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(4) << pos.X << "," << pos.Y << "," << pos.Z;

	msg += oss.str();
}

Position Navigation::CalculatePath(unsigned int mapID, Position start, Position end) {
	if (!is_socket_connected(sock) && !ConnectToServer("127.0.0.1", 50002)) {
		return start;
	}

    Position nextpos = start;
    if (end.DistanceTo(start) < 2.0f) return start;

    // Build and send: "C" + "sx,sy,sz ex,ey,ez mapID"
    std::string msg = "C";
    appendPosition(msg, start);
    msg += " ";
    appendPosition(msg, end);
	msg += " "+std::to_string(mapID);
    send(sock, msg.c_str(), (int)msg.size(), 0);

    // Receive reply "nx,ny,nz"
    char buffer[128];
    int r = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (r > 0) {
        buffer[r] = '\0'; // make it a C-string
        Position decoded{};
        if (parse_vec3_cstr(buffer, decoded)) {
            nextpos = decoded; // set the next waypoint/position from server
        }
    }

    return nextpos;
}

bool Navigation::ConnectToServer(const char* addr, int port) {
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

void Navigation::DisconnectClient() {
	closesocket(sock);
	WSACleanup();
	std::cout << "Navigation client disconnected !\n";
}