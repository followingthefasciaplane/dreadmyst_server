#include "stdafx.h"
#include "Player.h"
#include "WorldManager.h"
#include "MapManager.h"
#include "../Network/Session.h"
#include "../Handlers/WorldHandlers.h"
#include "../Systems/ExperienceSystem.h"
#include "../Systems/QuestManager.h"
#include "../Database/DatabaseManager.h"
#include "../Database/GameData.h"
#include "../Core/Logger.h"
#include "StlBuffer.h"
#include "GamePacketServer.h"

#include <chrono>
#include <cmath>

namespace {
    int64_t getCurrentTimeMs()
    {
        using namespace std::chrono;
        return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
    }
}

Player::Player(Session& session, const CharacterInfo& info)
    : m_session(session)
    , m_characterGuid(info.guid)
    , m_accountId(info.accountId)
    , m_characterName(info.name)
    , m_classId(info.classId)
    , m_gender(info.gender)
    , m_level(info.level)
    , m_experience(info.experience)
    , m_portraitId(info.portraitId)
    , m_skinColor(info.skinColor)
    , m_hairStyle(info.hairStyle)
    , m_hairColor(info.hairColor)
    , m_gold(info.gold)
    , m_playedTime(info.playedTime)
{
    // Set entity name
    setName(info.name);

    // Set position from database
    setPosition(info.mapId, info.posX, info.posY);
    setOrientation(info.facing);

    // Set initial stats using variable system
    setVariable(ObjDefines::Variable::Health, info.health);
    setVariable(ObjDefines::Variable::MaxHealth, info.maxHealth);
    setVariable(ObjDefines::Variable::Mana, info.mana);
    setVariable(ObjDefines::Variable::MaxMana, info.maxMana);
    setVariable(ObjDefines::Variable::Level, info.level);
    setVariable(ObjDefines::Variable::Experience, info.experience);
    setVariable(ObjDefines::Variable::Gold, info.gold);
    setVariable(ObjDefines::Variable::Progression, info.experience);

    // Record session start time for played time calculation
    m_sessionStartTime = getCurrentTimeMs();

    // Load inventory from database
    m_inventory.setOwner(this);
    m_inventory.load(m_characterGuid);

    // Load equipment from database
    m_equipment.load(m_characterGuid);

    // Load bank from database
    m_bank.load(m_characterGuid);

    // Load quest log from database (Phase 7)
    m_questLog.load(m_characterGuid);

    // Load stat bonuses (Phase 7 level-up)
    loadStatBonuses();

    // Sync quest item progress from inventory
    onInventoryChanged();

    // Initialize base stats from class stats (preserve current health/mana)
    sExperienceSystem.applyLevelStats(this, m_level, true, false);

    // Test character invulnerability: characters with names starting with "InvTester"
    // are made invulnerable for E2E testing purposes
    if (m_characterName.find("InvTester") == 0)
    {
        setInvulnerable(true);
        LOG_INFO("Player: '%s' marked as invulnerable (test character)", m_characterName.c_str());
    }

    LOG_DEBUG("Player: Created '%s' (guid=%d, class=%d, level=%d)",
              m_characterName.c_str(), m_characterGuid, m_classId, m_level);
}

Player::~Player()
{
    // Final save before destruction
    if (m_needsSave)
    {
        save();
    }

    LOG_DEBUG("Player: Destroyed '{}'", m_characterName);
}

void Player::update(float deltaTime)
{
    // Periodic position/data save (Task 4.9)
    if (m_needsSave)
    {
        m_saveTimer += deltaTime;
        if (m_saveTimer >= SAVE_INTERVAL)
        {
            save();
            m_saveTimer = 0.0f;
            LOG_DEBUG("Player: Periodic save for '%s' at (%.1f, %.1f)",
                      m_characterName.c_str(), getX(), getY());
        }
    }

    // Check pending cast timer
    if (m_pendingCast.active)
    {
        m_pendingCast.remainingTime -= deltaTime;
        if (m_pendingCast.remainingTime <= 0.0f)
        {
            completeCast();
        }
    }

    // Future: update movement interpolation, combat, buffs, etc.
}

void Player::sendPacket(const StlBuffer& packet)
{
    m_session.sendPacket(packet);
}

void Player::addExperience(int32_t amount)
{
    if (amount <= 0)
        return;

    m_experience += amount;
    setVariable(ObjDefines::Variable::Experience, m_experience);
    setVariable(ObjDefines::Variable::Progression, m_experience);
    markDirty();

    // TODO: Check for level up
}

void Player::setExperience(int32_t value)
{
    if (value < 0)
        value = 0;

    m_experience = value;
    setVariable(ObjDefines::Variable::Experience, m_experience);
    setVariable(ObjDefines::Variable::Progression, m_experience);
    markDirty();
}

void Player::setLevel(int32_t level)
{
    if (level < 1)
        level = 1;

    m_level = level;
    setVariable(ObjDefines::Variable::Level, m_level);
    markDirty();
}

void Player::addGold(int32_t amount)
{
    if (amount <= 0)
        return;

    m_gold += amount;
    setVariable(ObjDefines::Variable::Gold, m_gold);
    markDirty();
}

bool Player::spendGold(int32_t amount)
{
    if (amount <= 0)
        return true;

    if (m_gold < amount)
        return false;

    m_gold -= amount;
    setVariable(ObjDefines::Variable::Gold, m_gold);
    markDirty();
    return true;
}

void Player::save()
{
    LOG_DEBUG("Player: Saving '{}'...", m_characterName);

    // Update played time before saving
    updatePlayedTime();

    // Wrap all saves in a transaction for atomicity
    // This prevents partial saves and item duplication on crash
    sDatabase.beginTransaction();

    try
    {
        // Create CharacterInfo from current state
        CharacterInfo info = toCharacterInfo();

        // Save character data to database
        CharacterDb::saveCharacter(info);

        // Save inventory if dirty
        if (m_inventory.isDirty())
        {
            m_inventory.save(m_characterGuid);
            m_inventory.clearDirty();
        }

        // Save equipment if dirty
        if (m_equipment.isDirty())
        {
            m_equipment.save(m_characterGuid);
            m_equipment.clearDirty();
        }

        // Save bank if dirty
        if (m_bank.isDirty())
        {
            m_bank.save(m_characterGuid);
            m_bank.clearDirty();
        }

        // Save quest log if dirty (Phase 7)
        if (m_questLog.isDirty())
        {
            m_questLog.save(m_characterGuid);
            m_questLog.clearDirty();
        }

        // Save stat bonuses if dirty (Phase 7 level-up)
        if (m_statBonusesDirty)
            saveStatBonuses();

        // All saves successful - commit the transaction
        sDatabase.commit();
        m_needsSave = false;

        LOG_DEBUG("Player: Save committed for '{}'", m_characterName);
    }
    catch (const std::exception& e)
    {
        // Something went wrong - rollback all changes
        sDatabase.rollback();
        LOG_ERROR("Player: Save failed for '{}': {} - rolled back", m_characterName, e.what());
    }
}

void Player::loadFromDatabase()
{
    auto info = CharacterDb::getCharacterByGuid(m_characterGuid);
    if (!info)
    {
        LOG_ERROR("Player: Failed to reload '{}' from database", m_characterName);
        return;
    }

    // Update from database
    m_characterName = info->name;
    m_level = info->level;
    m_experience = info->experience;
    m_gold = info->gold;
    m_playedTime = info->playedTime;

    setPosition(info->mapId, info->posX, info->posY);
    setOrientation(info->facing);

    setVariable(ObjDefines::Variable::Health, info->health);
    setVariable(ObjDefines::Variable::MaxHealth, info->maxHealth);
    setVariable(ObjDefines::Variable::Mana, info->mana);
    setVariable(ObjDefines::Variable::MaxMana, info->maxMana);
    setVariable(ObjDefines::Variable::Progression, info->experience);
}

void Player::loadStatBonuses()
{
    m_statBonuses.clear();

    auto stmt = sDatabase.prepare(
        "SELECT stat_id, bonus FROM character_stat_bonuses WHERE character_guid = ?"
    );

    if (!stmt.valid())
    {
        LOG_ERROR("Player: Failed to prepare stat bonus load for %d", m_characterGuid);
        return;
    }

    stmt.bind(1, m_characterGuid);
    while (stmt.step())
    {
        int32_t statId = stmt.getInt(0);
        int32_t bonus = stmt.getInt(1);
        m_statBonuses[static_cast<UnitDefines::Stat>(statId)] = bonus;
    }

    m_statBonusesDirty = false;
    LOG_DEBUG("Player: Loaded %zu stat bonuses for %d", m_statBonuses.size(), m_characterGuid);
}

void Player::saveStatBonuses()
{
    auto deleteStmt = sDatabase.prepare(
        "DELETE FROM character_stat_bonuses WHERE character_guid = ?"
    );
    if (!deleteStmt.valid())
    {
        LOG_ERROR("Player: Failed to prepare stat bonus delete for %d", m_characterGuid);
        return;
    }

    deleteStmt.bind(1, m_characterGuid);
    deleteStmt.step();

    auto insertStmt = sDatabase.prepare(
        "INSERT INTO character_stat_bonuses (character_guid, stat_id, bonus) VALUES (?, ?, ?)"
    );
    if (!insertStmt.valid())
    {
        LOG_ERROR("Player: Failed to prepare stat bonus insert for %d", m_characterGuid);
        return;
    }

    for (const auto& [stat, bonus] : m_statBonuses)
    {
        insertStmt.reset();
        insertStmt.bind(1, m_characterGuid);
        insertStmt.bind(2, static_cast<int32_t>(stat));
        insertStmt.bind(3, bonus);
        insertStmt.step();
    }

    m_statBonusesDirty = false;
    LOG_DEBUG("Player: Saved %zu stat bonuses for %d", m_statBonuses.size(), m_characterGuid);
}

void Player::onInventoryChanged()
{
    sQuestManager.onInventoryChanged(this);
}

int32_t Player::getStatBonus(UnitDefines::Stat stat) const
{
    auto it = m_statBonuses.find(stat);
    return it != m_statBonuses.end() ? it->second : 0;
}

void Player::addStatBonus(UnitDefines::Stat stat, int32_t amount)
{
    if (amount == 0)
        return;

    int32_t next = getStatBonus(stat) + amount;
    if (next < 0)
        next = 0;

    setStatBonus(stat, next);
}

void Player::setStatBonus(UnitDefines::Stat stat, int32_t value)
{
    if (value <= 0)
        m_statBonuses.erase(stat);
    else
        m_statBonuses[stat] = value;

    m_statBonusesDirty = true;
}

int32_t Player::getEquipmentStatBonus(UnitDefines::Stat stat) const
{
    auto equipBonuses = m_equipment.calculateStatBonuses();
    auto it = equipBonuses.find(static_cast<int32_t>(stat));
    return it != equipBonuses.end() ? it->second : 0;
}

int32_t Player::getTotalStatBonus(UnitDefines::Stat stat) const
{
    int32_t total = 0;

    // Manual stat investments (from level-up)
    total += getStatBonus(stat);

    // Equipment bonuses
    total += getEquipmentStatBonus(stat);

    // Aura bonuses (buffs/debuffs)
    total += m_auras.getStatModifier(static_cast<int32_t>(stat));

    return total;
}

void Player::recalculateStats()
{
    // Reapply all stats with current equipment and buffs
    sExperienceSystem.applyLevelStats(this, m_level, true, true);

    LOG_DEBUG("Player: Recalculated stats for %s", m_characterName.c_str());
}

bool Player::hasShieldEquipped() const
{
    const auto* offhand = m_equipment.getItem(UnitDefines::EquipSlot::Offhand);
    if (!offhand)
        return false;

    const ItemTemplate* tmpl = sGameData.getItem(offhand->itemId);
    if (!tmpl)
        return false;

    // Check if it's a shield (equipType == Shield)
    return tmpl->equipType == Equipment::EquipType::Shield;
}

bool Player::hasWeaponEquipped() const
{
    return !m_equipment.isSlotEmpty(UnitDefines::EquipSlot::Weapon1);
}

int32_t Player::getWeaponDamage() const
{
    const auto* weapon = m_equipment.getItem(UnitDefines::EquipSlot::Weapon1);
    if (!weapon)
        return 0;

    const ItemTemplate* tmpl = sGameData.getItem(weapon->itemId);
    if (!tmpl)
        return 0;

    // Get WeaponValue stat from item (stat_type 11 = WeaponValue)
    for (int i = 0; i < 10; ++i)
    {
        if (tmpl->statType[i] == static_cast<int32_t>(UnitDefines::Stat::WeaponValue))
            return tmpl->statValue[i];
    }

    return 0;
}

int32_t Player::getArmorValue() const
{
    // Sum armor from all equipped pieces + base ArmorValue stat
    int32_t armor = getTotalStatBonus(UnitDefines::Stat::ArmorValue);

    // Could also check for armor-type items here if needed
    return armor;
}

CharacterInfo Player::toCharacterInfo() const
{
    CharacterInfo info;
    info.guid = m_characterGuid;
    info.accountId = m_accountId;
    info.name = m_characterName;
    info.classId = m_classId;
    info.gender = m_gender;
    info.level = m_level;
    info.experience = m_experience;
    info.portraitId = m_portraitId;
    info.skinColor = m_skinColor;
    info.hairStyle = m_hairStyle;
    info.hairColor = m_hairColor;
    info.mapId = getMapId();
    info.posX = getX();
    info.posY = getY();
    info.facing = getOrientation();
    info.health = getHealth();
    info.maxHealth = getMaxHealth();
    info.mana = getMana();
    info.maxMana = getMaxMana();
    info.gold = m_gold;
    info.playedTime = m_playedTime;
    return info;
}

void Player::updatePlayedTime()
{
    int64_t now = getCurrentTimeMs();
    int64_t sessionDuration = (now - m_sessionStartTime) / 1000;  // Convert to seconds
    m_playedTime += static_cast<int32_t>(sessionDuration);
    m_sessionStartTime = now;  // Reset for next interval
}

bool Player::isInCombat() const
{
    return getVariable(ObjDefines::Variable::InCombat) != 0;
}

// ============================================================================
// Death Handling (Task 5.11)
// ============================================================================

void Player::onDeath(Entity* killer)
{
    // Call base class implementation
    Entity::onDeath(killer);

    LOG_INFO("Player '%s' died (killer=%lu)", m_characterName.c_str(),
             killer ? killer->getGuid() : 0);

    // Stop movement
    setMoving(false);

    // Clear combat state
    setVariable(ObjDefines::Variable::InCombat, 0);

    // Reduce equipment durability on death
    if (m_equipment.reduceDurabilityOnDeath())
    {
        // Send equipment update to client so they see the durability change
        sendEquipment();
    }

    // Mark for save (death state should be saved)
    markDirty();
}

// ============================================================================
// Respawn System (Task 5.12)
// ============================================================================

bool Player::canRespawn() const
{
    // Must be dead to respawn
    return isDead();
}

void Player::respawn()
{
    if (!canRespawn())
    {
        LOG_WARN("Player '%s' attempted to respawn but is not dead", m_characterName.c_str());
        return;
    }

    LOG_INFO("Player '%s' respawning", m_characterName.c_str());

    // Get respawn position - for now use the map's starting position
    // TODO: In future, find nearest graveyard
    float respawnX = getX();
    float respawnY = getY();
    float respawnOrientation = getOrientation();

    // Try to get map's default spawn point
    sMapManager.getStartPosition(getMapId(), respawnX, respawnY, respawnOrientation);

    // Restore health to 50%
    int32_t maxHealth = getMaxHealth();
    int32_t respawnHealth = maxHealth / 2;
    if (respawnHealth < 1) respawnHealth = 1;

    // Restore mana to 50%
    int32_t maxMana = getMaxMana();
    int32_t respawnMana = maxMana / 2;

    // Clear dead state and restore stats
    setDead(false);
    setVariable(ObjDefines::Variable::Health, respawnHealth);
    setVariable(ObjDefines::Variable::Mana, respawnMana);

    // Broadcast the new health/mana values
    broadcastVariable(ObjDefines::Variable::Health, respawnHealth);
    broadcastVariable(ObjDefines::Variable::Mana, respawnMana);

    // Teleport to respawn position
    teleportTo(respawnX, respawnY);
    setOrientation(respawnOrientation);

    // Send respawn response
    GP_Server_RespawnResponse response;
    response.m_success = true;

    StlBuffer buf;
    uint16_t opcode = response.getOpcode();
    buf << opcode;
    response.pack(buf);
    sendPacket(buf);

    LOG_INFO("Player '%s' respawned at (%.1f, %.1f) with %d health",
             m_characterName.c_str(), respawnX, respawnY, respawnHealth);

    markDirty();
}

// ============================================================================
// Position Sync (Task 4.9)
// ============================================================================

void Player::teleportTo(float x, float y)
{
    setPosition(x, y);
    broadcastTeleport();
    markDirty();

    LOG_DEBUG("Player: '%s' teleported to (%.1f, %.1f)", m_characterName.c_str(), x, y);
}

void Player::teleportTo(int mapId, float x, float y)
{
    if (mapId != getMapId())
    {
        // Cross-map teleport - use WorldManager
        sWorldManager.changePlayerMap(this, mapId, x, y, getOrientation());
    }
    else
    {
        // Same map teleport
        teleportTo(x, y);
    }
}

void Player::broadcastTeleport()
{
    // Use GP_Server_UnitSpline with m_slide=true for instant teleport
    // The client binary does NOT have a handler for GP_Server_UnitTeleport,
    // but GP_Server_UnitSpline with slide=true achieves instant position change
    GP_Server_UnitSpline packet;
    packet.m_guid = static_cast<uint32_t>(getGuid());
    packet.m_startX = getX();
    packet.m_startY = getY();
    packet.m_spline.push_back({getX(), getY()});  // Single point = instant position set
    packet.m_slide = true;   // Slide mode = instant/forced movement (no walk animation)
    packet.m_silent = false;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);

    // Send to self (client needs to know its corrected position)
    sendPacket(buf);

    // Send to all viewers
    sWorldManager.broadcastToVisible(this, buf, false);
}

void Player::updateOrientationFromMovement(float destX, float destY)
{
    float dx = destX - getX();
    float dy = destY - getY();

    // Only update if moving a significant distance
    if (std::abs(dx) < 1.0f && std::abs(dy) < 1.0f)
        return;

    // Calculate angle in radians (atan2 gives angle from positive X axis)
    float angle = std::atan2(dy, dx);

    setOrientation(angle);

    // Note: GP_Server_UnitOrientation is NOT supported by the client binary.
    // The client calculates orientation from movement spline direction automatically.
    // Orientation is synced when the full GP_Server_Player packet is sent (on spawn/respawn).
}

// ============================================================================
// Cooldown System (Task 5.7)
// ============================================================================

void Player::startCooldown(int32_t spellId, int32_t durationMs, int32_t categoryId)
{
    if (durationMs <= 0)
        return;

    // Start the cooldown internally
    m_cooldowns.startCooldown(spellId, durationMs, categoryId);

    // Send cooldown notification to client
    GP_Server_Cooldown packet;
    packet.m_id = spellId;
    packet.m_totalDuration = durationMs;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);

    sendPacket(buf);

    LOG_DEBUG("Player: '{}' started cooldown for spell {} ({}ms)",
              m_characterName, spellId, durationMs);
}

void Player::startGCD()
{
    m_cooldowns.startGCD();

    // GCD is typically not sent as a separate packet - client handles it locally
    // based on spell cast. However, if needed we can send a special indicator.
    // For now, just start it internally for server-side validation.
}

void Player::sendAllCooldowns()
{
    auto cooldowns = m_cooldowns.getAllCooldowns();

    for (const auto& [spellId, remainingMs] : cooldowns)
    {
        GP_Server_Cooldown packet;
        packet.m_id = spellId;
        packet.m_totalDuration = remainingMs;

        StlBuffer buf;
        uint16_t opcode = packet.getOpcode();
        buf << opcode;
        packet.pack(buf);

        sendPacket(buf);
    }

    LOG_DEBUG("Player: '{}' sent {} active cooldowns", m_characterName, cooldowns.size());
}

void Player::sendInventory()
{
    GP_Server_Inventory packet;
    packet.m_gold = m_gold;

    const auto& slots = m_inventory.getSlots();
    for (int i = 0; i < Inventory::MAX_SLOTS; ++i)
    {
        const auto& item = slots[i];
        if (!item.isEmpty())
        {
            GP_Server_Inventory::Slot slot;
            slot.slot = i;
            slot.itemId.m_itemId = item.itemId;
            slot.itemId.m_affix1 = item.affix1;
            slot.itemId.m_affix2 = item.affix2;
            slot.itemId.m_gem1 = item.gem1;
            slot.itemId.m_gem2 = item.gem2;
            slot.itemId.m_gem3 = item.gem3;
            slot.itemId.m_gem4 = 0;
            slot.itemId.m_durability = item.durability;
            slot.itemId.m_enchant = item.enchantId;
            slot.itemId.m_count = item.stackCount;
            slot.stackCount = item.stackCount;
            packet.m_slots.push_back(slot);
        }
    }

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);

    sendPacket(buf);

    LOG_DEBUG("Player: '{}' sent inventory ({} items, {} gold)",
              m_characterName, packet.m_slots.size(), m_gold);
}

void Player::sendEquipment()
{
    const auto& slots = m_equipment.getSlots();
    int sentCount = 0;

    for (int i = 0; i < Equipment::NUM_SLOTS; ++i)
    {
        const auto& item = slots[i];
        if (!item.isEmpty())
        {
            GP_Server_EquipItem packet;
            packet.m_guid = static_cast<uint32_t>(getGuid());
            packet.m_slot = i;
            packet.m_itemId.m_itemId = item.itemId;
            packet.m_itemId.m_affix1 = item.affix1;
            packet.m_itemId.m_affix2 = item.affix2;
            packet.m_itemId.m_gem1 = item.gem1;
            packet.m_itemId.m_gem2 = item.gem2;
            packet.m_itemId.m_gem3 = item.gem3;
            packet.m_itemId.m_gem4 = 0;
            packet.m_itemId.m_durability = item.durability;
            packet.m_itemId.m_enchant = item.enchantId;
            packet.m_silent = true;

            StlBuffer buf;
            uint16_t opcode = packet.getOpcode();
            buf << opcode;
            packet.pack(buf);

            sendPacket(buf);
            ++sentCount;
        }
    }

    LOG_DEBUG("Player: '{}' sent equipment ({} items)",
              m_characterName, sentCount);
}

void Player::broadcastEquipmentChange(UnitDefines::EquipSlot slot)
{
    GP_Server_EquipItem packet;
    packet.m_guid = static_cast<uint32_t>(getGuid());
    packet.m_slot = static_cast<int32_t>(slot);

    const Equipment::EquippedItem* item = m_equipment.getItem(slot);
    if (item)
    {
        packet.m_itemId.m_itemId = item->itemId;
        packet.m_itemId.m_affix1 = item->affix1;
        packet.m_itemId.m_affix2 = item->affix2;
        packet.m_itemId.m_gem1 = item->gem1;
        packet.m_itemId.m_gem2 = item->gem2;
        packet.m_itemId.m_gem3 = item->gem3;
        packet.m_itemId.m_gem4 = 0;
        packet.m_itemId.m_durability = item->durability;
        packet.m_itemId.m_enchant = item->enchantId;
    }
    else
    {
        // Empty slot - send item ID of 0
        packet.m_itemId.m_itemId = 0;
    }
    packet.m_silent = false;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);

    // Send to self
    sendPacket(buf);

    // Broadcast to all visible players
    sWorldManager.broadcastToVisible(this, buf, false);

    LOG_DEBUG("Player: '{}' broadcast equipment change (slot={}, itemId={})",
              m_characterName, static_cast<int>(slot), packet.m_itemId.m_itemId);
}

// ============================================================================
// Cast System (cast time spells)
// ============================================================================

void Player::startCast(int32_t spellId, uint32_t targetGuid, float castTime)
{
    // Cancel any existing cast
    if (m_pendingCast.active)
    {
        cancelCast();
    }

    m_pendingCast.spellId = spellId;
    m_pendingCast.targetGuid = targetGuid;
    m_pendingCast.remainingTime = castTime / 1000.0f;  // Convert ms to seconds
    m_pendingCast.active = true;

    LOG_DEBUG("Player: '{}' started casting spell {} (cast time: {} ms)",
              m_characterName, spellId, castTime);
}

void Player::cancelCast()
{
    if (!m_pendingCast.active)
        return;

    int32_t cancelledSpell = m_pendingCast.spellId;
    m_pendingCast.active = false;
    m_pendingCast.spellId = 0;
    m_pendingCast.targetGuid = 0;
    m_pendingCast.remainingTime = 0.0f;

    // Send cast stop to client and nearby players
    GP_Server_CastStop stopPacket;
    stopPacket.m_guid = static_cast<uint32_t>(getGuid());
    stopPacket.m_spellId = cancelledSpell;

    StlBuffer buf;
    uint16_t opcode = stopPacket.getOpcode();
    buf << opcode;
    stopPacket.pack(buf);

    sendPacket(buf);
    sWorldManager.broadcastToVisible(this, buf, false);

    LOG_DEBUG("Player: '{}' cancelled cast of spell {}", m_characterName, cancelledSpell);
}

void Player::completeCast()
{
    if (!m_pendingCast.active)
        return;

    // Copy cast info and clear pending (prevent re-entry)
    int32_t spellId = m_pendingCast.spellId;
    uint32_t targetGuid = m_pendingCast.targetGuid;
    m_pendingCast.active = false;
    m_pendingCast.spellId = 0;
    m_pendingCast.targetGuid = 0;
    m_pendingCast.remainingTime = 0.0f;

    LOG_DEBUG("Player: '{}' completed cast of spell {} on target {}",
              m_characterName, spellId, targetGuid);

    // Execute the spell through the handler system
    Handlers::executePendingCast(this, spellId, targetGuid);
}
