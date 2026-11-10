#include "Navigation.h"
#include "DangerZoneGeometry.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>

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
    constexpr auto FailedPathCooldown = std::chrono::milliseconds(400);
    constexpr auto ReconnectCooldown = std::chrono::milliseconds(400);
    constexpr size_t MaxFrameSize = 4096;
    std::mutex NetworkMutex;
    Clock::time_point ReconnectAfter{};
    bool WinsockActive = false;
    std::string ReceivedData;
    // Local avoidance must clear immediately; server removals can be retried later.
    // Protected by NetworkMutex.
    std::vector<std::string> PendingBlacklistRemovals;

    void ForgetFailedPath() {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        LastFailedPath.Valid = false;
    }
    bool RecentlyFailed(unsigned int mapId, const Position& start, const Position& end) {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        return LastFailedPath.Valid && LastFailedPath.MapId == mapId &&
            Clock::now() < LastFailedPath.RetryAfter &&
            start.DistanceTo2D(LastFailedPath.Start) <= 4.0f &&
            end.DistanceTo2D(LastFailedPath.Goal) <= 3.0f;
    }
    void NoteFailed(unsigned int mapId, const Position& start, const Position& end) {
        std::lock_guard<std::mutex> guard(PathRetryMutex);
        LastFailedPath = {mapId, start, end, Clock::now() + FailedPathCooldown, true};
    }
}
Position Navigation::LastMoveDestination;
bool Navigation::HasLastMove = false;
MovementWatchdog Navigation::MoveWatchdog;

// Navigation requests share a single persistent, synchronized TCP connection.
static bool SendAll(SOCKET socketHandle, const std::string& message) {
    size_t sent = 0;
    while (sent < message.size()) {
        const int count = send(socketHandle, message.data() + sent,
            static_cast<int>(message.size() - sent), 0);
        if (count <= 0) return false;
        sent += static_cast<size_t>(count);
    }
    return true;
}

static void CloseNavigationSocket(SOCKET& socketHandle) {
    if (socketHandle != INVALID_SOCKET) {
        closesocket(socketHandle);
        socketHandle = INVALID_SOCKET;
    }
    ReceivedData.clear();
}

static void RetryNavigationLater(SOCKET& socketHandle) {
    CloseNavigationSocket(socketHandle);
    ReconnectAfter = Clock::now() + ReconnectCooldown;
}

// Ancienne reception par buffer C (conservee pour reference) :
// make it a C-string
static bool ReceiveLine(SOCKET socketHandle, std::string& line) {
    while (true) {
        const size_t newline = ReceivedData.find('\n');
        if (newline != std::string::npos) {
            line = ReceivedData.substr(0, newline);
            ReceivedData.erase(0, newline + 1);
            if (!line.empty() && line.back() == '\r') line.pop_back();
            return true;
        }
        if (ReceivedData.size() >= MaxFrameSize) return false;
        char buffer[256];
        const int count = recv(socketHandle, buffer, sizeof(buffer), 0);
        if (count <= 0) return false;
        ReceivedData.append(buffer, static_cast<size_t>(count));
    }
}

static bool EnsureNavigationConnected(SOCKET& socketHandle, WSADATA& wsaData, const char* addr, int port) {
    if (socketHandle != INVALID_SOCKET) return true;
    if (Clock::now() < ReconnectAfter) return false;

    if (!WinsockActive) {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            ReconnectAfter = Clock::now() + ReconnectCooldown;
            return false;
        }
        WinsockActive = true;
    }

    socketHandle = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socketHandle == INVALID_SOCKET) {
        ReconnectAfter = Clock::now() + ReconnectCooldown;
        return false;
    }
    const DWORD receiveTimeoutMs = 1000;
    const DWORD sendTimeoutMs = 600;
    setsockopt(socketHandle, SOL_SOCKET, SO_RCVTIMEO,
        reinterpret_cast<const char*>(&receiveTimeoutMs), sizeof(receiveTimeoutMs));
    setsockopt(socketHandle, SOL_SOCKET, SO_SNDTIMEO,
        reinterpret_cast<const char*>(&sendTimeoutMs), sizeof(sendTimeoutMs));

    SOCKADDR_IN sin{};
    sin.sin_addr.s_addr = inet_addr(addr);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(static_cast<u_short>(port));
    if (connect(socketHandle, reinterpret_cast<SOCKADDR*>(&sin), sizeof(sin)) == SOCKET_ERROR) {
        RetryNavigationLater(socketHandle);
        return false;
    }
    return true;
}

// Called with NetworkMutex held, after the socket is connected. On a disconnect,
// keep unconfirmed removals so a later route request can synchronize the server.
static bool FlushPendingBlacklistRemovals(SOCKET& socketHandle) {
    while (!PendingBlacklistRemovals.empty()) {
        const std::string command = "R " + PendingBlacklistRemovals.back() + "\n";
        std::string reply;
        if (!SendAll(socketHandle, command) || !ReceiveLine(socketHandle, reply)) {
            RetryNavigationLater(socketHandle);
            return false;
        }
        if (reply != "OK") return false;
        PendingBlacklistRemovals.pop_back();
    }
    return true;
}

void appendPosition(std::string& msg, const Position& pos) {
	// truncate at 4 decimals
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(4) << pos.X << "," << pos.Y << "," << pos.Z;

	msg += oss.str();
}

Position Navigation::CalculatePath(unsigned int mapID, Position start, Position end, bool throttleFailedRequests) {
    if (!std::isfinite(start.X) || !std::isfinite(start.Y) || !std::isfinite(start.Z) ||
        !std::isfinite(end.X) || !std::isfinite(end.Y) || !std::isfinite(end.Z)) return start;
    if (end.DistanceTo(start) < 2.0f) return start;
    if (throttleFailedRequests && RecentlyFailed(mapID, start, end)) return start;

    std::lock_guard<std::mutex> networkGuard(NetworkMutex);
    if (!EnsureNavigationConnected(sock, WSAData, "127.0.0.1", 50002) ||
        !FlushPendingBlacklistRemovals(sock)) {
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }

    // Build and send: "C" + "sx,sy,sz ex,ey,ez mapID"
    std::string msg = "C";
    appendPosition(msg, start);
    msg += " ";
    appendPosition(msg, end);
    msg += " " + std::to_string(mapID) + "\n";
    // Receive reply "nx,ny,nz"
    std::string response;
    if (!SendAll(sock, msg) || !ReceiveLine(sock, response)) {
        RetryNavigationLater(sock);
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }

    if (response == "ERROR") {
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }

    Position waypoint{};
    if (std::sscanf(response.c_str(), " %f , %f , %f", &waypoint.X, &waypoint.Y, &waypoint.Z) != 3 ||
        !std::isfinite(waypoint.X) || !std::isfinite(waypoint.Y) || !std::isfinite(waypoint.Z)) {
        RetryNavigationLater(sock);
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }
    if (waypoint.DistanceTo(start) < 2.0f) {
        if (throttleFailedRequests) NoteFailed(mapID, start, end);
        return start;
    }
    ForgetFailedPath();
    return waypoint;
}

static bool ReceiveNavigationAck(SOCKET socketHandle, bool& transportOk) {
    std::string response;
    transportOk = ReceiveLine(socketHandle, response);
    return transportOk && response == "OK";
}

bool Navigation::AddBlacklist(unsigned int mapId, const std::string& name, const Position& position, float radius, unsigned int type) {
    if (name.empty() || radius <= 0.0f) return false;
    // Add locally *first*: even a disconnected navigation server must never
    // cause an explicitly blacklisted zone to be ignored by CTM movement.
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        if (std::any_of(Blacklists.begin(), Blacklists.end(),
            [&](const BlacklistEntry& entry) { return entry.Name == name; })) return false;
        Blacklists.push_back({name, position, radius, mapId});
        ++ZoneVersion;
    }
    ForgetFailedPath();

    std::lock_guard<std::mutex> networkGuard(NetworkMutex);
    if (!EnsureNavigationConnected(sock, WSAData, "127.0.0.1", 50002) ||
        !FlushPendingBlacklistRemovals(sock)) return false;
    std::ostringstream oss;
    oss << "B " << mapId << " " << name << " "
        << std::fixed << std::setprecision(4)
        << position.X << "," << position.Y << "," << position.Z << " "
        << radius << " " << type << "\n";
    bool transportOk = false;
    const bool sent = SendAll(sock, oss.str());
    const bool synced = sent && ReceiveNavigationAck(sock, transportOk);
    if (!sent || !transportOk) RetryNavigationLater(sock);
    return synced;
}

bool Navigation::RemoveBlacklist(const std::string& name) {
    bool synced = false;
    {
        std::lock_guard<std::mutex> networkGuard(NetworkMutex);
        if (EnsureNavigationConnected(sock, WSAData, "127.0.0.1", 50002) &&
            FlushPendingBlacklistRemovals(sock)) {
            bool transportOk = false;
            const bool sent = SendAll(sock, "R " + name + "\n");
            synced = sent && ReceiveNavigationAck(sock, transportOk);
            if (!sent || !transportOk) RetryNavigationLater(sock);
        }
        if (!synced && std::find(PendingBlacklistRemovals.begin(),
                PendingBlacklistRemovals.end(), name) == PendingBlacklistRemovals.end())
            PendingBlacklistRemovals.push_back(name);
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

bool Navigation::HasBlacklists() {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return !Blacklists.empty();
}

void Navigation::ClearBlacklists() {
    std::vector<std::string> names;
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        if (Blacklists.empty()) return;
        names.reserve(Blacklists.size());
        for (const auto& entry : Blacklists) names.push_back(entry.Name);
        Blacklists.clear();
        ++ZoneVersion;
    }
    // Never block chores or keep a stale danger area just because the server is
    // unavailable. The next navigation request synchronizes these removals.
    {
        std::lock_guard<std::mutex> networkGuard(NetworkMutex);
        for (const auto& name : names) {
            if (std::find(PendingBlacklistRemovals.begin(),
                    PendingBlacklistRemovals.end(), name) == PendingBlacklistRemovals.end())
                PendingBlacklistRemovals.push_back(name);
        }
    }
    ForgetFailedPath();
}

void Navigation::SetNpcDangerZones(unsigned int mapId, const std::vector<DangerZone>& zones) {
    std::vector<DangerZone> filtered;
    filtered.reserve(zones.size());
    for (const auto& zone : zones)
        if (zone.MapId == mapId && zone.Radius > 0.0f) filtered.push_back(zone);

    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    const bool unchanged = std::equal(filtered.begin(), filtered.end(),
        NpcDangerZones.begin(), NpcDangerZones.end(),
        [](const DangerZone& a, const DangerZone& b) {
            return a.Guid == b.Guid && a.MapId == b.MapId && a.Radius == b.Radius &&
                a.Center == b.Center;
        });
    if (unchanged) return;
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

bool Navigation::IsCurrentMovementStalled(const Position& from, bool movementReported) {
    std::lock_guard<std::mutex> guard(AvoidanceMutex);
    return HasLastMove && MoveWatchdog.Stalled(from, MovementWatchdog::Clock::now(), movementReported);
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
    if (from.DistanceTo2D(destination) < 1.0f) return false;
    return !IsSegmentSafe(mapId, from, destination);
}

bool Navigation::ConnectToServer(const char* addr, int port) {
    std::lock_guard<std::mutex> networkGuard(NetworkMutex);
    return EnsureNavigationConnected(sock, WSAData, addr, port);
}

void Navigation::DisconnectClient() {
    std::lock_guard<std::mutex> networkGuard(NetworkMutex);
    CloseNavigationSocket(sock);
    if (WinsockActive) {
        WSACleanup();
        WinsockActive = false;
    }
    ReconnectAfter = {};
    ForgetFailedPath();
}
