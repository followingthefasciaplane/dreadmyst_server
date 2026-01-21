// WorldManager - Manages all entities in the game world
// Task 4.6: Player Spawn/Despawn
// Task 4.7: Visibility/Interest System
// Task 5.14: NPC Management

#include "stdafx.h"
#include "World/WorldManager.h"
#include "World/Player.h"
#include "World/Npc.h"
#include "World/NpcSpawner.h"
#include "Systems/QuestManager.h"
#include "Systems/DuelSystem.h"
#include "Network/Session.h"
#include "Core/Logger.h"
#include "GamePacketServer.h"
#include "StlBuffer.h"
#include "ObjDefines.h"

#include <cmath>

WorldManager& WorldManager::instance()
{
    static WorldManager instance;
    return instance;
}

void WorldManager::initialize()
{
    LOG_INFO("WorldManager initialized");
}

void WorldManager::shutdown()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Note: We don't delete players here - Session owns them
    m_players.clear();
    m_playersByMap.clear();

    LOG_INFO("WorldManager shutdown");
}

// ============================================================================
// Player Management
// ============================================================================

void WorldManager::addPlayer(Player* player)
{
    if (!player)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t guid = player->getGuid();
    int mapId = player->getMapId();

    // Add to global map
    m_players[guid] = player;

    // Add to per-map set
    m_playersByMap[mapId].insert(player);

    LOG_DEBUG("WorldManager: Added player '%s' (guid=%u) to map %d. Total players: %zu",
              player->getName().c_str(), guid, mapId, m_players.size());
}

void WorldManager::removePlayer(Player* player)
{
    if (!player)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t guid = player->getGuid();
    int mapId = player->getMapId();

    // Remove from global map
    m_players.erase(guid);

    // Remove from per-map set
    auto mapIt = m_playersByMap.find(mapId);
    if (mapIt != m_playersByMap.end())
    {
        mapIt->second.erase(player);

        // Clean up empty map entries
        if (mapIt->second.empty())
        {
            m_playersByMap.erase(mapIt);
        }
    }

    LOG_DEBUG("WorldManager: Removed player '%s' (guid=%u) from map %d. Total players: %zu",
              player->getName().c_str(), guid, mapId, m_players.size());
}

Player* WorldManager::getPlayer(uint32_t guid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_players.find(guid);
    return it != m_players.end() ? it->second : nullptr;
}

Player* WorldManager::getPlayerByName(const std::string& name) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    for (const auto& [guid, player] : m_players)
    {
        if (player->getName() == name)
            return player;
    }
    return nullptr;
}

std::vector<Player*> WorldManager::getPlayersOnMap(int mapId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Player*> result;
    auto it = m_playersByMap.find(mapId);
    if (it != m_playersByMap.end())
    {
        result.reserve(it->second.size());
        for (Player* p : it->second)
        {
            result.push_back(p);
        }
    }
    return result;
}

std::vector<Player*> WorldManager::getAllPlayers() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Player*> result;
    result.reserve(m_players.size());
    for (const auto& [guid, player] : m_players)
    {
        result.push_back(player);
    }
    return result;
}

size_t WorldManager::getPlayerCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_players.size();
}

size_t WorldManager::getPlayerCountOnMap(int mapId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_playersByMap.find(mapId);
    return it != m_playersByMap.end() ? it->second.size() : 0;
}

// ============================================================================
// Spawn/Despawn with Broadcasts
// ============================================================================

void WorldManager::spawnPlayer(Player* player)
{
    if (!player)
        return;

    int mapId = player->getMapId();

    // First, add to tracking (must be done before getting other players)
    addPlayer(player);

    // Get all other players on the same map
    std::vector<Player*> playersOnMap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_playersByMap.find(mapId);
        if (it != m_playersByMap.end())
        {
            for (Player* p : it->second)
            {
                if (p != player)
                    playersOnMap.push_back(p);
            }
        }
    }

    // Count how many players we actually notify (for logging)
    size_t visibleCount = 0;

    // For each player on the map, check visibility and exchange spawn packets
    for (Player* other : playersOnMap)
    {
        if (canPlayersSeeEachOther(player, other))
        {
            // Set up mutual visibility tracking
            player->addCanSee(other);
            other->addVisibleTo(player);
            other->addCanSee(player);
            player->addVisibleTo(other);

            // Send this new player to the other player
            sendPlayerTo(other, player);

            // Send the other player to the new player
            sendPlayerTo(player, other);

            visibleCount++;
        }
    }

    // Send all NPCs on this map to the new player (Task 5.14)
    std::vector<Npc*> npcsOnMap = getNpcsOnMap(mapId);
    for (Npc* npc : npcsOnMap)
    {
        if (npc->isSpawned())
        {
            sendNpcTo(player, npc);
        }
    }

    LOG_INFO("WorldManager: Player '%s' spawned on map %d. Visible to %zu of %zu players, %zu NPCs.",
             player->getName().c_str(), mapId, visibleCount, playersOnMap.size(), npcsOnMap.size());
}

void WorldManager::despawnPlayer(Player* player)
{
    if (!player)
        return;

    int mapId = player->getMapId();
    uint32_t guid = player->getGuid();

    // Get all players who can see this player (from visibility sets)
    // We use visibleTo because those are the players who need to be notified
    std::vector<Player*> viewers;
    {
        const auto& visibleTo = player->getVisibleTo();
        viewers.reserve(visibleTo.size());

        for (Entity* entity : visibleTo)
        {
            if (Player* viewer = dynamic_cast<Player*>(entity))
            {
                viewers.push_back(viewer);
            }
        }
    }

    // Clean up visibility tracking from both sides
    for (Player* viewer : viewers)
    {
        // Remove this player from viewer's canSee set
        viewer->removeCanSee(player);

        // Send destroy packet to viewer
        sendDestroyTo(viewer, guid);
    }

    // Also clean up the canSee set (players this player was watching)
    for (Entity* entity : player->getCanSee())
    {
        entity->removeVisibleTo(player);
    }

    // Clear this player's visibility sets
    player->clearCanSee();
    player->clearVisibleTo();

    // Remove from tracking
    removePlayer(player);

    LOG_INFO("WorldManager: Player '%s' despawned from map %d. Notified %zu viewers.",
             player->getName().c_str(), mapId, viewers.size());
}

void WorldManager::changePlayerMap(Player* player, int newMapId, float x, float y, float orientation)
{
    if (!player)
        return;

    int oldMapId = player->getMapId();
    uint32_t guid = player->getGuid();

    // Skip if same map
    if (oldMapId == newMapId)
    {
        player->setPosition(x, y);
        player->setOrientation(orientation);
        return;
    }

    // Clean up visibility on old map - notify all viewers and clean up tracking
    std::vector<Player*> oldViewers;
    {
        const auto& visibleTo = player->getVisibleTo();
        for (Entity* entity : visibleTo)
        {
            if (Player* viewer = dynamic_cast<Player*>(entity))
            {
                oldViewers.push_back(viewer);
            }
        }
    }

    // Notify viewers on old map and clean up their tracking
    for (Player* viewer : oldViewers)
    {
        viewer->removeCanSee(player);
        sendDestroyTo(viewer, guid);
    }

    // Clean up players this player was watching
    for (Entity* entity : player->getCanSee())
    {
        entity->removeVisibleTo(player);
    }

    // Clear this player's visibility sets
    player->clearCanSee();
    player->clearVisibleTo();

    // Update per-map tracking
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playersByMap[oldMapId].erase(player);
        if (m_playersByMap[oldMapId].empty())
            m_playersByMap.erase(oldMapId);
    }

    // Update player position
    player->setPosition(newMapId, x, y);
    player->setOrientation(orientation);

    // Add to new map tracking and get players on new map
    std::vector<Player*> newMapPlayers;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_playersByMap[newMapId].insert(player);

        auto it = m_playersByMap.find(newMapId);
        if (it != m_playersByMap.end())
        {
            for (Player* p : it->second)
            {
                if (p != player)
                    newMapPlayers.push_back(p);
            }
        }
    }

    // Send NewWorld to the player
    {
        GP_Server_NewWorld packet;
        packet.m_mapId = newMapId;

        StlBuffer buf;
        uint16_t opcode = packet.getOpcode();
        buf << opcode;
        packet.pack(buf);
        player->sendPacket(buf);
    }

    // Set up visibility on new map with range checking
    size_t visibleCount = 0;
    for (Player* other : newMapPlayers)
    {
        if (canPlayersSeeEachOther(player, other))
        {
            // Set up mutual visibility tracking
            player->addCanSee(other);
            other->addVisibleTo(player);
            other->addCanSee(player);
            player->addVisibleTo(other);

            // Exchange spawn packets
            sendPlayerTo(other, player);
            sendPlayerTo(player, other);

            visibleCount++;
        }
    }

    LOG_INFO("WorldManager: Player '%s' changed map %d -> %d at (%.1f, %.1f). Visible to %zu of %zu players.",
             player->getName().c_str(), oldMapId, newMapId, x, y, visibleCount, newMapPlayers.size());
}

// ============================================================================
// Broadcasting
// ============================================================================

void WorldManager::broadcastToMap(int mapId, const StlBuffer& packet, Player* excludePlayer)
{
    std::vector<Player*> recipients;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_playersByMap.find(mapId);
        if (it != m_playersByMap.end())
        {
            for (Player* p : it->second)
            {
                if (p != excludePlayer)
                    recipients.push_back(p);
            }
        }
    }

    for (Player* recipient : recipients)
    {
        recipient->sendPacket(packet);
    }
}

void WorldManager::broadcastGlobal(const StlBuffer& packet, Player* excludePlayer)
{
    std::vector<Player*> recipients;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (const auto& [guid, player] : m_players)
        {
            if (player != excludePlayer)
                recipients.push_back(player);
        }
    }

    for (Player* recipient : recipients)
    {
        recipient->sendPacket(packet);
    }
}

// ============================================================================
// Update
// ============================================================================

void WorldManager::update(float deltaTime)
{
    // Get snapshot of players and NPCs to avoid holding lock during update
    std::vector<Player*> players;
    std::vector<Npc*> npcs;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        players.reserve(m_players.size());
        for (const auto& [guid, player] : m_players)
        {
            players.push_back(player);
        }

        npcs.reserve(m_npcs.size());
        for (const auto& [guid, npc] : m_npcs)
        {
            npcs.push_back(npc.get());
        }
    }

    // Update all players
    for (Player* player : players)
    {
        player->update(deltaTime);
    }

    // Update all NPCs (Task 5.14)
    for (Npc* npc : npcs)
    {
        if (!npc->isSpawned())
            continue;

        npc->update(deltaTime);
    }

    // Update NPC respawn timers (Task 7.1)
    sNpcSpawner.update(deltaTime);

    // Update duel system (Task 8.8)
    sDuelManager.update(deltaTime);
}

// ============================================================================
// Visibility System (Task 4.7)
// ============================================================================

bool WorldManager::canPlayersSeeEachOther(Player* a, Player* b) const
{
    if (!a || !b)
        return false;

    // Must be on same map
    if (a->getMapId() != b->getMapId())
        return false;

    // If view distance is 0 or negative, unlimited visibility (whole map)
    if (WORLD_VIEW_DISTANCE <= 0.0f)
        return true;

    // Check distance
    return a->isInRange(b, WORLD_VIEW_DISTANCE);
}

void WorldManager::updateVisibility(Player* player)
{
    if (!player)
        return;

    int mapId = player->getMapId();

    // Get all other players on same map
    std::vector<Player*> playersOnMap;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_playersByMap.find(mapId);
        if (it != m_playersByMap.end())
        {
            for (Player* p : it->second)
            {
                if (p != player)
                    playersOnMap.push_back(p);
            }
        }
    }

    // Check each player on the map
    for (Player* other : playersOnMap)
    {
        bool shouldSee = canPlayersSeeEachOther(player, other);
        bool currentlySees = player->getCanSee().count(other) > 0;

        if (shouldSee && !currentlySees)
        {
            // Player just came into view - send spawn packets
            player->addCanSee(other);
            other->addVisibleTo(player);

            // Send other player to this player
            sendPlayerTo(player, other);

            // Send this player to other player (mutual visibility)
            other->addCanSee(player);
            player->addVisibleTo(other);
            sendPlayerTo(other, player);

            LOG_DEBUG("Visibility: '%s' and '%s' can now see each other",
                      player->getName().c_str(), other->getName().c_str());
        }
        else if (!shouldSee && currentlySees)
        {
            // Player left view range - send despawn packets
            player->removeCanSee(other);
            other->removeVisibleTo(player);

            // Despawn other player for this player
            sendDestroyTo(player, other->getGuid());

            // Despawn this player for other player (mutual)
            other->removeCanSee(player);
            player->removeVisibleTo(other);
            sendDestroyTo(other, player->getGuid());

            LOG_DEBUG("Visibility: '%s' and '%s' can no longer see each other",
                      player->getName().c_str(), other->getName().c_str());
        }
    }
}

void WorldManager::onPlayerMoved(Player* player, float oldX, float oldY)
{
    if (!player)
        return;

    // Calculate how far the player moved
    float dx = player->getX() - oldX;
    float dy = player->getY() - oldY;
    float distMoved = std::sqrt(dx * dx + dy * dy);

    // Only update visibility if player moved a significant distance
    // This prevents constant visibility recalculation during smooth movement
    // Use 10% of view distance as threshold, or 50 pixels if unlimited view
    float threshold = (WORLD_VIEW_DISTANCE > 0.0f) ? WORLD_VIEW_DISTANCE * 0.1f : 50.0f;

    if (distMoved >= threshold)
    {
        updateVisibility(player);
    }
}

void WorldManager::broadcastToVisible(Player* player, const StlBuffer& packet, bool includeSelf)
{
    if (!player)
        return;

    // Get list of players who can see this player
    std::vector<Player*> viewers;
    {
        // Copy the visibleTo set to avoid issues with concurrent modification
        const auto& visibleTo = player->getVisibleTo();
        viewers.reserve(visibleTo.size() + 1);

        for (Entity* entity : visibleTo)
        {
            if (Player* viewer = dynamic_cast<Player*>(entity))
            {
                viewers.push_back(viewer);
            }
        }
    }

    // Send to all viewers
    for (Player* viewer : viewers)
    {
        viewer->sendPacket(packet);
    }

    // Optionally send to self
    if (includeSelf)
    {
        player->sendPacket(packet);
    }
}

// ============================================================================
// Internal Helpers
// ============================================================================

void WorldManager::sendPlayerTo(Player* target, Player* playerToSend)
{
    if (!target || !playerToSend)
        return;

    GP_Server_Player packet;
    packet.m_guid = playerToSend->getGuid();
    packet.m_name = playerToSend->getName();
    packet.m_subName = "";
    packet.m_classId = static_cast<uint8_t>(playerToSend->getClassId());
    packet.m_gender = static_cast<uint8_t>(playerToSend->getGender());
    packet.m_portraitId = playerToSend->getPortraitId();
    packet.m_x = playerToSend->getX();
    packet.m_y = playerToSend->getY();
    packet.m_orientation = playerToSend->getOrientation();

    // Variables
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Health)] = playerToSend->getHealth();
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::MaxHealth)] = playerToSend->getMaxHealth();
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Mana)] = playerToSend->getMana();
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::MaxMana)] = playerToSend->getMaxMana();
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Level)] = playerToSend->getLevel();

    // TODO: Equipment in Phase 6

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    target->sendPacket(buf);
}

void WorldManager::sendDestroyTo(Player* target, uint32_t guid)
{
    if (!target)
        return;

    GP_Server_DestroyObject packet;
    packet.m_guid = guid;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    target->sendPacket(buf);
}

// ============================================================================
// NPC Management (Task 5.14)
// ============================================================================

Npc* WorldManager::spawnNpc(const NpcTemplate& tmpl, int mapId, float x, float y, float orientation)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // Generate unique GUID for NPC
    uint32_t guid = m_nextNpcGuid++;

    // Create NPC
    auto npc = std::make_unique<Npc>(tmpl, mapId, x, y, orientation);
    npc->setGuid(guid);
    npc->setSpawned(true);

    Npc* npcPtr = npc.get();

    // Store in maps
    m_npcs[guid] = std::move(npc);
    m_npcsByMap[mapId].insert(npcPtr);

    LOG_DEBUG("WorldManager: Spawned NPC '{}' (entry={}, guid={}) at map {} ({:.1f}, {:.1f})",
              npcPtr->getName(), tmpl.entry, guid, mapId, x, y);

    return npcPtr;
}

void WorldManager::despawnNpc(Npc* npc)
{
    if (!npc)
        return;

    // Note: NPC is kept for respawn, just marked as despawned
    npc->setSpawned(false);

    // Broadcast despawn to all players on the map
    broadcastNpcDespawn(npc);

    LOG_DEBUG("WorldManager: Despawned NPC '{}' (guid={})",
              npc->getName(), npc->getGuid());
}

void WorldManager::removeNpc(Npc* npc)
{
    if (!npc)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    uint32_t guid = npc->getGuid();
    int mapId = npc->getMapId();

    // Remove from per-map tracking
    auto mapIt = m_npcsByMap.find(mapId);
    if (mapIt != m_npcsByMap.end())
    {
        mapIt->second.erase(npc);
        if (mapIt->second.empty())
            m_npcsByMap.erase(mapIt);
    }

    // Remove from main map (this deletes the NPC)
    m_npcs.erase(guid);

    LOG_DEBUG("WorldManager: Removed NPC guid={}", guid);
}

Npc* WorldManager::getNpc(uint32_t guid) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_npcs.find(guid);
    return it != m_npcs.end() ? it->second.get() : nullptr;
}

std::vector<Npc*> WorldManager::getNpcsOnMap(int mapId) const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Npc*> result;
    auto it = m_npcsByMap.find(mapId);
    if (it != m_npcsByMap.end())
    {
        result.reserve(it->second.size());
        for (Npc* npc : it->second)
        {
            result.push_back(npc);
        }
    }
    return result;
}

std::vector<Npc*> WorldManager::getAllNpcs() const
{
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<Npc*> result;
    result.reserve(m_npcs.size());
    for (const auto& [guid, npc] : m_npcs)
    {
        result.push_back(npc.get());
    }
    return result;
}

size_t WorldManager::getNpcCount() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_npcs.size();
}

void WorldManager::sendNpcTo(Player* target, Npc* npc)
{
    if (!target || !npc || !npc->isSpawned())
        return;

    GP_Server_Npc packet;
    packet.m_guid = npc->getGuid();
    packet.m_entry = npc->getEntry();
    packet.m_x = npc->getX();
    packet.m_y = npc->getY();
    packet.m_orientation = npc->getOrientation();

    // Send key variables
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Health)] =
        npc->getVariable(ObjDefines::Variable::Health);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::MaxHealth)] =
        npc->getVariable(ObjDefines::Variable::MaxHealth);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Level)] =
        npc->getVariable(ObjDefines::Variable::Level);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Faction)] =
        npc->getVariable(ObjDefines::Variable::Faction);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::ModelId)] =
        npc->getVariable(ObjDefines::Variable::ModelId);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::ModelScale)] =
        npc->getVariable(ObjDefines::Variable::ModelScale);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::MoveSpeedPct)] =
        npc->getVariable(ObjDefines::Variable::MoveSpeedPct);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::DynGossipStatus)] =
        static_cast<int32_t>(sQuestManager.getGossipStatus(target, npc));
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Elite)] =
        npc->getVariable(ObjDefines::Variable::Elite);
    packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Boss)] =
        npc->getVariable(ObjDefines::Variable::Boss);

    // Mana if applicable
    int32_t maxMana = npc->getVariable(ObjDefines::Variable::MaxMana);
    if (maxMana > 0)
    {
        packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::Mana)] =
            npc->getVariable(ObjDefines::Variable::Mana);
        packet.m_variables[static_cast<int32_t>(ObjDefines::Variable::MaxMana)] = maxMana;
    }

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    target->sendPacket(buf);
}

void WorldManager::broadcastNpcSpawn(Npc* npc)
{
    if (!npc || !npc->isSpawned())
        return;

    int mapId = npc->getMapId();
    std::vector<Player*> playersOnMap = getPlayersOnMap(mapId);

    for (Player* player : playersOnMap)
    {
        sendNpcTo(player, npc);
    }

    LOG_DEBUG("WorldManager: Broadcast NPC '{}' spawn to {} players",
              npc->getName(), playersOnMap.size());
}

void WorldManager::broadcastNpcDespawn(Npc* npc)
{
    if (!npc)
        return;

    int mapId = npc->getMapId();
    uint32_t guid = npc->getGuid();

    std::vector<Player*> playersOnMap = getPlayersOnMap(mapId);

    for (Player* player : playersOnMap)
    {
        sendDestroyTo(player, guid);
    }
}

// ============================================================================
// Entity Update Broadcasts (for variable changes)
// The client does NOT support GP_Server_ObjectVariable, so we resend the
// full entity packet to synchronize variable changes like IsDead, flags, etc.
// ============================================================================

void WorldManager::broadcastPlayerUpdate(Player* player)
{
    if (!player)
        return;

    // Resend full player packet to all viewers (includes updated variables)
    std::vector<Player*> playersOnMap = getPlayersOnMap(player->getMapId());

    for (Player* viewer : playersOnMap)
    {
        // Send to self and all other players
        sendPlayerTo(viewer, player);
    }

    LOG_DEBUG("WorldManager: Broadcast player '{}' update to {} viewers",
              player->getName(), playersOnMap.size());
}

void WorldManager::broadcastNpcUpdate(Npc* npc)
{
    if (!npc || !npc->isSpawned())
        return;

    // Resend full NPC packet to all players on map (includes updated variables)
    int mapId = npc->getMapId();
    std::vector<Player*> playersOnMap = getPlayersOnMap(mapId);

    for (Player* player : playersOnMap)
    {
        sendNpcTo(player, npc);
    }

    LOG_DEBUG("WorldManager: Broadcast NPC '{}' update to {} players",
              npc->getName(), playersOnMap.size());
}
