#include "Navigation.h"
#include "DangerZoneGeometry.h"
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
    constexpr auto ReconnectCooldown = std::chrono::milliseconds(400);
    constexpr size_t MaxFrameSize = 4096;
    std::mutex NetworkMutex;
    Clock::time_point ReconnectAfter{};
    bool WinsockActive = false;
    std::string ReceivedData;
    std::vector<std::string> PendingBlacklistRemovals;
}

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
	std::ostringstream oss;
	oss << std::fixed << std::setprecision(4) << pos.X << "," << pos.Y << "," << pos.Z;

	msg += oss.str();
}

Position Navigation::CalculatePath(unsigned int mapID, Position start, Position end) {
    if (!std::isfinite(start.X) || !std::isfinite(start.Y) || !std::isfinite(start.Z) ||
        !std::isfinite(end.X) || !std::isfinite(end.Y) || !std::isfinite(end.Z)) return start;
    if (end.DistanceTo(start) < 2.0f) return start;

    std::lock_guard<std::mutex> networkGuard(NetworkMutex);
    if (!EnsureNavigationConnected(sock, WSAData, "127.0.0.1", 50002) ||
        !FlushPendingBlacklistRemovals(sock)) return start;

    std::string msg = "C";
    appendPosition(msg, start);
    msg += " ";
    appendPosition(msg, end);
    msg += " " + std::to_string(mapID) + "\n";
    std::string response;
    if (!SendAll(sock, msg) || !ReceiveLine(sock, response)) {
        RetryNavigationLater(sock);
        return start;
    }

    if (response == "ERROR") return start;

    Position waypoint{};
    if (std::sscanf(response.c_str(), " %f , %f , %f", &waypoint.X, &waypoint.Y, &waypoint.Z) != 3 ||
        !std::isfinite(waypoint.X) || !std::isfinite(waypoint.Y) || !std::isfinite(waypoint.Z)) {
        RetryNavigationLater(sock);
        return start;
    }
    if (waypoint.DistanceTo(start) < 2.0f) return start;
    return waypoint;
}

static bool ReceiveNavigationAck(SOCKET socketHandle, bool& transportOk) {
    std::string response;
    transportOk = ReceiveLine(socketHandle, response);
    return transportOk && response == "OK";
}

bool Navigation::AddBlacklist(unsigned int mapId, const std::string& name, const Position& position, float radius, unsigned int type) {
    if (name.empty() || radius <= 0.0f) return false;
    {
        std::lock_guard<std::mutex> guard(AvoidanceMutex);
        if (std::any_of(Blacklists.begin(), Blacklists.end(),
            [&](const BlacklistEntry& entry) { return entry.Name == name; })) return false;
        Blacklists.push_back({name, position, radius, mapId});
        ++ZoneVersion;
    }

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
    {
        std::lock_guard<std::mutex> networkGuard(NetworkMutex);
        for (const auto& name : names) {
            if (std::find(PendingBlacklistRemovals.begin(),
                    PendingBlacklistRemovals.end(), name) == PendingBlacklistRemovals.end())
                PendingBlacklistRemovals.push_back(name);
        }
    }
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
}
