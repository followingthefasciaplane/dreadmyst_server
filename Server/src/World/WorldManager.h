// WorldManager - Manages all entities in the game world
// Task 4.6: Player Spawn/Despawn
// Task 4.7: Visibility/Interest System
// Task 5.14: NPC Management

#pragma once

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <mutex>
#include <memory>

class Player;
class Entity;
class Npc;
struct NpcTemplate;

// View distance for visibility system (in pixels)
// Set to 0 for unlimited visibility (all players on same map can see each other)
// Typical values: 800-1200 for limited visibility, 0 for full map visibility
constexpr float WORLD_VIEW_DISTANCE = 0.0f;

// Manages all entities in the world, tracks players per map,
// handles spawn/despawn broadcasts with visibility filtering
class WorldManager
{
public:
    static WorldManager& instance();

    // Lifecycle
    void initialize();
    void shutdown();

    // Player management
    void addPlayer(Player* player);
    void removePlayer(Player* player);
    Player* getPlayer(uint32_t guid) const;
    Player* getPlayerByName(const std::string& name) const;

    // Get all players on a specific map
    std::vector<Player*> getPlayersOnMap(int mapId) const;

    // Get all players (for global broadcasts)
    std::vector<Player*> getAllPlayers() const;

    // Get player count
    size_t getPlayerCount() const;
    size_t getPlayerCountOnMap(int mapId) const;

    // Spawn/Despawn with broadcasts
    // spawnPlayer: Broadcasts this player to all others on same map,
    //              and sends all existing players on map to this player
    void spawnPlayer(Player* player);

    // despawnPlayer: Broadcasts DestroyObject to all others on same map
    void despawnPlayer(Player* player);

    // Map transitions
    void changePlayerMap(Player* player, int newMapId, float x, float y, float orientation);

    // Update all entities (called each tick)
    void update(float deltaTime);

    // Visibility system (Task 4.7)
    // Updates which players can see which - call after significant movement
    void updateVisibility(Player* player);

    // Called when a player moves - updates visibility and broadcasts position
    void onPlayerMoved(Player* player, float oldX, float oldY);

    // Check if two players can see each other (based on view distance)
    bool canPlayersSeeEachOther(Player* a, Player* b) const;

    // Broadcast to all players on a map (except sender)
    void broadcastToMap(int mapId, const class StlBuffer& packet, Player* excludePlayer = nullptr);

    // Broadcast to all players who can see the given player
    void broadcastToVisible(Player* player, const class StlBuffer& packet, bool includeSelf = false);

    // Broadcast to all players globally (except sender)
    void broadcastGlobal(const class StlBuffer& packet, Player* excludePlayer = nullptr);

    // =========================================================================
    // NPC Management (Task 5.14)
    // =========================================================================

    // Spawn an NPC from template at position
    Npc* spawnNpc(const NpcTemplate& tmpl, int mapId, float x, float y, float orientation = 0.0f);

    // Despawn an NPC (removes from world, keeps for respawn)
    void despawnNpc(Npc* npc);

    // Remove NPC completely (for shutdown)
    void removeNpc(Npc* npc);

    // Get NPC by GUID
    Npc* getNpc(uint32_t guid) const;

    // Get all NPCs on a specific map
    std::vector<Npc*> getNpcsOnMap(int mapId) const;

    // Get all NPCs
    std::vector<Npc*> getAllNpcs() const;

    // Get NPC count
    size_t getNpcCount() const;

    // Send NPC spawn packet to a player
    void sendNpcTo(Player* target, Npc* npc);

    // Broadcast NPC to all players on map
    void broadcastNpcSpawn(Npc* npc);

    // Broadcast NPC despawn to all players on map
    void broadcastNpcDespawn(Npc* npc);

    // Broadcast entity updates (resend full entity packet for variable changes)
    // Used when GP_Server_ObjectVariable is not supported by client
    void broadcastPlayerUpdate(Player* player);
    void broadcastNpcUpdate(Npc* npc);

private:
    WorldManager() = default;
    ~WorldManager() = default;
    WorldManager(const WorldManager&) = delete;
    WorldManager& operator=(const WorldManager&) = delete;

    // Send player packet to a specific player (for spawn visibility)
    void sendPlayerTo(Player* target, Player* playerToSend);

    // Send destroy packet to a specific player
    void sendDestroyTo(Player* target, uint32_t guid);

    // All players by GUID
    std::unordered_map<uint32_t, Player*> m_players;

    // Players grouped by map ID for efficient map-local operations
    std::unordered_map<int, std::unordered_set<Player*>> m_playersByMap;

    // All NPCs by GUID (owns the Npc objects)
    std::unordered_map<uint32_t, std::unique_ptr<Npc>> m_npcs;

    // NPCs grouped by map ID for efficient map-local operations
    std::unordered_map<int, std::unordered_set<Npc*>> m_npcsByMap;

    // GUID counter for NPCs (uses high bits to distinguish from players)
    uint32_t m_nextNpcGuid = 0x80000000;  // Start NPCs at high GUID range

    // Thread safety
    mutable std::mutex m_mutex;
};

#define sWorldManager WorldManager::instance()
