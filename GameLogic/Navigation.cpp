#include "Navigation.h"
#include "DangerZoneGeometry.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <chrono>

SOCKET Navigation::sock = INVALID_SOCKET;
WSADATA Navigation::WSAData;
std::vector<BlacklistEntry> Navigation::Blacklists;
std::vector<DangerZone> Navigation::NpcDangerZones;
std::mutex Navigation::AvoidanceMutex;
std::uint64_t Navigation::ZoneVersion = 1;
std::unordered_map<unsigned int, std::pair<std::uint64_t, Navigation::AvoidanceSnapshot>> Navigation::ZoneSnapshotCache;

namespace {
    using Clock = std::chrono::steady_clock;
    struct FailedPathRequest {
        unsigned int MapId = 0;
        Position Start{};
        Position Goal{};
        Clock::time_point RetryAfter{};
        bool Valid = false;
    };
    std::mutex PathRetryMutex;
    FailedPathRequest LastFailedPath;
    constexpr auto FailedPathCooldown = std::chrono::milliseconds(1250);

    float DistanceSq2D(const Position& a, const Position& b) {
        const float dx = a.X - b.X, dy = a.Y - b.Y;
        return dx * dx + dy * dy;
    }
    void ForgetFailedPath() {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        LastFailedPath.Valid = false;
    }
    bool RecentlyFailed(unsigned int mapId, const Position& start, const Position& end) {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        return LastFailedPath.Valid && LastFailedPath.MapId == mapId &&
            Clock::now() < LastFailedPath.RetryAfter &&
            DistanceSq2D(start, LastFailedPath.Start) <= 16.0f &&
            DistanceSq2D(end, LastFailedPath.Goal) <= 9.0f;
    }
    void NoteFailed(unsigned int mapId, const Position& start, const Position& end) {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        LastFailedPath = {mapId, start, end, Clock::now() + FailedPathCooldown, true};
    }
}
Position Navigation::LastMoveDestination;
bool Navigation::HasLastMove = false;
MovementWatchdog Navigation::MoveWatchdog;

static void CloseNavigationSocket(SOCKET& socketHandle) {
    if (socketHandle != INVALID_SOCKET) {
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
    }
    WSACleanup();
}

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

Position Navigation::CalculatePath(unsigned int mapID, Position start, Position end, bool throttleFailedRequests) {
    if (end.DistanceTo(start) < 2.0f) return start;
    if (throttleFailedRequests && RecentlyFailed(mapID, start, end)) return start;
    if (!is_socket_connected(sock) && !ConnectToServer("127.0.0.1", 50002)) {
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }

    // Build and send: "C" + "sx,sy,sz ex,ey,ez mapID"
    std::string msg = "C";
    appendPosition(msg, start);
    msg += " ";
    appendPosition(msg, end);
	msg += " "+std::to_string(mapID);
    if (send(sock, msg.c_str(), (int)msg.size(), 0) != static_cast<int>(msg.size())) {
        CloseNavigationSocket(sock);
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }

    // Receive reply "nx,ny,nz"
    char buffer[128];
    int r = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (r > 0) {
        buffer[r] = '\0'; // make it a C-string
        Position decoded{};
        if (parse_vec3_cstr(buffer, decoded) &&
            std::isfinite(decoded.X) && std::isfinite(decoded.Y) && std::isfinite(decoded.Z)) {
            if (decoded.DistanceTo(start) < 2.0f) {
                if (throttleFailedRequests) NoteFailed(mapID, start, end);
                return start;
            }
            if (throttleFailedRequests) ForgetFailedPath();
            return decoded;
        }
    }
    CloseNavigationSocket(sock);
    if (throttleFailedRequests) NoteFailed(mapID, start, end);
    return start;
}

static bool ReceiveNavigationAck(SOCKET sock) {
    char buffer[16];
    int r = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (r <= 0) return false;
    buffer[r] = '\0';
    return std::string(buffer) == "OK";
}

bool Navigation::AddBlacklist(unsigned int mapId, const std::string& name, const Position& position, float radius, unsigned int type) {
    if (name.empty() || radius <= 0.0f || IsBlacklisted(name)) return false;

    // Add locally *first*: even a disconnected navigation server must never
    // cause an explicitly blacklisted zone to be ignored by CTM movement.
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        Blacklists.push_back({name, position, radius, mapId});
        ++ZoneVersion;
    }
    ForgetFailedPath();

    if (!is_socket_connected(sock) && !ConnectToServer("127.0.0.1", 50002)) return false;
    std::ostringstream oss;
    oss << "B " << mapId << " " << name << " "
        << std::fixed << std::setprecision(4)
        << position.X << "," << position.Y << "," << position.Z << " "
        << radius << " " << type;
    const std::string msg = oss.str();
    const bool synced = send(sock, msg.c_str(), static_cast<int>(msg.size()), 0) ==
        static_cast<int>(msg.size()) && ReceiveNavigationAck(sock);
    if (!synced) CloseNavigationSocket(sock);
    return synced;
}

bool Navigation::RemoveBlacklist(const std::string& name) {
    bool synced = false;
    if (is_socket_connected(sock) || ConnectToServer("127.0.0.1", 50002)) {
        const std::string msg = "R " + name;
        synced = send(sock, msg.c_str(), static_cast<int>(msg.size()), 0) ==
                 static_cast<int>(msg.size()) && ReceiveNavigationAck(sock);
        if (!synced) CloseNavigationSocket(sock);
    }
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        const auto oldSize = Blacklists.size();
        Blacklists.erase(std::remove_if(Blacklists.begin(), Blacklists.end(),
            [&](const BlacklistEntry& entry) { return entry.Name == name; }), Blacklists.end());
        if (Blacklists.size() != oldSize) ++ZoneVersion;
    }
    ForgetFailedPath();
    return synced;
}

bool Navigation::IsBlacklisted(const std::string& name) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    for (const auto& blacklist : Blacklists) {
		if(blacklist.Name == name) return true;
    }

    return false;
}

bool Navigation::HasBlacklists() {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return !Blacklists.empty();
}

void Navigation::ClearBlacklists() {
    while (true) {
        std::string name;
        {
            std::lock_guard<std::mutex> guard(AvoidanceMutex);
            if (Blacklists.empty()) break;
            name = Blacklists.back().Name;
        }

        if (!RemoveBlacklist(name)) {
            std::cout << "Warning: server blacklist not removed: " << name << "\n";
        }
    }
}

void Navigation::SetNpcDangerZones(unsigned int mapId, const std::vector<DangerZone>& zones) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    std::size_t index = 0;
    bool changed = false;
    for (const auto& zone : zones) {
        if (!(zone.Radius > 0.0f) || zone.MapId != mapId) continue;
        if (index == NpcDangerZones.size()) {
            changed = true;
            break;
        }
        const auto& previous = NpcDangerZones[index++];
        if (zone.Guid != previous.Guid || zone.MapId != previous.MapId ||
            zone.Radius != previous.Radius || zone.Center.X != previous.Center.X ||
            zone.Center.Y != previous.Center.Y || zone.Center.Z != previous.Center.Z) {
            changed = true;
            break;
        }
    }
    if (!changed && index == NpcDangerZones.size()) return;

    std::vector<DangerZone> filtered;
    filtered.reserve(zones.size());
    for (const auto& zone : zones)
        if (zone.Radius > 0.0f && zone.MapId == mapId) filtered.push_back(zone);
    NpcDangerZones.swap(filtered);
    ++ZoneVersion;
}

Navigation::AvoidanceSnapshot Navigation::GetAvoidanceSnapshot(unsigned int mapId) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    auto& cached = ZoneSnapshotCache[mapId];
    if (!cached.second || cached.first != ZoneVersion) {
        auto snapshot = std::make_shared<std::vector<DangerZone>>();
        snapshot->reserve(NpcDangerZones.size() + Blacklists.size());
        for (const auto& zone : NpcDangerZones)
            if (zone.MapId == mapId) snapshot->push_back(zone);
        for (const auto& entry : Blacklists)
            if (entry.MapId == mapId)
                snapshot->push_back({0, entry.MapId, entry.Center, entry.Radius});
        cached = {ZoneVersion, snapshot};
    }
    return cached.second;
}

std::vector<DangerZone> Navigation::GetAvoidanceZones(unsigned int mapId) {
    return *GetAvoidanceSnapshot(mapId);
}

bool Navigation::HasAvoidanceZones(unsigned int mapId) {
    return !GetAvoidanceSnapshot(mapId)->empty();
}

bool Navigation::IsPositionDangerous(const AvoidanceSnapshot& zones, const Position& position) {
    if (!zones) return false;
    for (const auto& zone : *zones)
        if (DangerZoneGeometry::Contains(position, zone.Center, zone.Radius + DangerZoneGeometry::SafetyMargin))
            return true;
    return false;
}

bool Navigation::IsSegmentSafe(const AvoidanceSnapshot& zones, const Position& from, const Position& to) {
    if (!zones) return true;
    for (const auto& zone : *zones)
        if (DangerZoneGeometry::BlocksSegment(from, to, zone.Center, zone.Radius + DangerZoneGeometry::SafetyMargin))
            return false;
    return true;
}

bool Navigation::IsPositionDangerous(unsigned int mapId, const Position& position) {
    return IsPositionDangerous(GetAvoidanceSnapshot(mapId), position);
}

bool Navigation::IsSegmentSafe(unsigned int mapId, const Position& from, const Position& to) {
    return IsSegmentSafe(GetAvoidanceSnapshot(mapId), from, to);
}

void Navigation::RecordMovement(const Position& start, const Position& destination) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    LastMoveDestination = destination;
    HasLastMove = true;
    MoveWatchdog.Record(start, destination, MovementWatchdog::Clock::now());
}

bool Navigation::HasCurrentMovement() {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return HasLastMove && MoveWatchdog.Active();
}

bool Navigation::IsMovingTowards(const Position& destination) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return HasLastMove && MoveWatchdog.Towards(destination);
}

bool Navigation::GetCurrentMovementDestination(Position& destination) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    if (!HasLastMove) return false;
    destination = LastMoveDestination;
    return true;
}

bool Navigation::IsCurrentMovementStalled(const Position& from) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return HasLastMove && MoveWatchdog.Stalled(from, MovementWatchdog::Clock::now());
}

void Navigation::ClearMovement() {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    HasLastMove = false;
    MoveWatchdog.Clear();
}

bool Navigation::IsCurrentMovementUnsafe(unsigned int mapId, const Position& from) {
    Position destination;
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        if (!HasLastMove) return false;
        destination = LastMoveDestination;
    }
    const float dx = from.X - destination.X, dy = from.Y - destination.Y;
    if (dx * dx + dy * dy < 1.0f) return false;
    return !IsSegmentSafe(mapId, from, destination);
}

bool Navigation::ConnectToServer(const char* addr, int port) {
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) return false;
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr(addr);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) { WSACleanup(); return false; }

	std::cout << "Connecting to Navigation server...\n";
	if (connect(sock, (SOCKADDR*)&sin, sizeof(sin)) == SOCKET_ERROR) {
        CloseNavigationSocket(sock);
		return false;
	}
    const DWORD timeoutMs = 1500;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO,
        reinterpret_cast<const char*>(&timeoutMs), sizeof(timeoutMs));
	std::cout << "Connected to " << addr << ":" << port << "\n";
	return true;
}

void Navigation::DisconnectClient() {
    CloseNavigationSocket(sock);
	std::cout << "Navigation client disconnected !\n";
}