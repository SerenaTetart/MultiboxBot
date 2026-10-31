#pragma once

#include <windows.h>

#include "WoWObject.h"
#include "MovementWatchdog.h"

#include <vector>
#include <string>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <cstdint>

struct BlacklistEntry
{
    std::string Name;
    Position Center;
    float Radius;
    unsigned int MapId;
};

struct DangerZone {
    unsigned long long Guid;
    unsigned int MapId;
    Position Center;
    float Radius;
};

class Navigation {
public:
    using AvoidanceSnapshot = std::shared_ptr<const std::vector<DangerZone>>;
    static bool ConnectToServer(const char* addr, int port);

    static Position CalculatePath(
        unsigned int mapId,
        const Position start,
        const Position end,
        bool throttleFailedRequests = false // combat only; fast follow/journey bypass backoff
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

    static void SetNpcDangerZones(unsigned int mapId, const std::vector<DangerZone>& zones);
    static AvoidanceSnapshot GetAvoidanceSnapshot(unsigned int mapId);
    static std::vector<DangerZone> GetAvoidanceZones(unsigned int mapId);
    static bool HasAvoidanceZones(unsigned int mapId);
    static bool IsPositionDangerous(unsigned int mapId, const Position& position);
    static bool IsSegmentSafe(unsigned int mapId, const Position& from, const Position& to);
    static bool IsSegmentSafe(const AvoidanceSnapshot& zones, const Position& from, const Position& to);
    static bool IsPositionDangerous(const AvoidanceSnapshot& zones, const Position& position);

    static void RecordMovement(const Position& start, const Position& destination);
    static bool HasCurrentMovement();
    static bool IsMovingTowards(const Position& destination);
    static bool IsCurrentMovementStalled(const Position& from);
    static bool GetCurrentMovementDestination(Position& destination);
    static void ClearMovement();
    static bool IsCurrentMovementUnsafe(unsigned int mapId, const Position& from);
    static void DisconnectClient();
private:
	static bool IsBlacklisted(const std::string& name);
    static bool is_socket_connected(SOCKET s);

    static WSADATA WSAData;
    static SOCKET sock;
	static std::vector<BlacklistEntry> Blacklists;
    static std::vector<DangerZone> NpcDangerZones;
    static std::mutex AvoidanceMutex;
    static std::uint64_t ZoneVersion;
    static std::unordered_map<unsigned int, std::pair<std::uint64_t, AvoidanceSnapshot>> ZoneSnapshotCache;
    static Position LastMoveDestination;
    static bool HasLastMove;
    static MovementWatchdog MoveWatchdog;
};