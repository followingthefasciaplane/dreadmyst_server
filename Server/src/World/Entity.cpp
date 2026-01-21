#include "stdafx.h"
#include "Entity.h"
#include "Map.h"
#include "Player.h"
#include "Npc.h"
#include "WorldManager.h"
#include "Systems/DuelSystem.h"
#include "Core/Logger.h"
#include "GamePacketServer.h"

#include <cmath>
#include <algorithm>

// GUID generation counter (simple incremental for now)
static uint32_t s_nextGuid = 1;

Entity::Entity()
{
    setGuid(s_nextGuid++);

    // Set aura manager owner (Task 5.8)
    m_auras.setOwner(this);
}

void Entity::setPosition(int mapId, float x, float y)
{
    m_mapId = mapId;
    m_x = x;
    m_y = y;
}

void Entity::setPosition(float x, float y)
{
    m_x = x;
    m_y = y;
}

int32_t Entity::getVariable(ObjDefines::Variable var) const
{
    auto it = m_variables.find(static_cast<int>(var));
    return it != m_variables.end() ? it->second : 0;
}

void Entity::setVariable(ObjDefines::Variable var, int32_t value)
{
    int32_t oldValue = getVariable(var);
    m_variables[static_cast<int>(var)] = value;

    // Notify callback if value changed
    if (m_variableCallback && oldValue != value)
        m_variableCallback(this, var, oldValue, value);
}

void Entity::modifyVariable(ObjDefines::Variable var, int32_t delta)
{
    setVariable(var, getVariable(var) + delta);
}

float Entity::distanceTo(const Entity* other) const
{
    if (!other)
        return 0.0f;
    return distanceTo(other->getX(), other->getY());
}

float Entity::distanceTo(float x, float y) const
{
    float dx = m_x - x;
    float dy = m_y - y;
    return std::sqrt(dx * dx + dy * dy);
}

bool Entity::isInRange(const Entity* other, float range) const
{
    return distanceTo(other) <= range;
}

bool Entity::isInRange(float x, float y, float range) const
{
    return distanceTo(x, y) <= range;
}

// ============================================================================
// Combat - Death Handling (Task 5.11)
// ============================================================================

bool Entity::isDead() const
{
    // Check both the IsDead flag and health value
    return getVariable(ObjDefines::Variable::IsDead) != 0 ||
           getVariable(ObjDefines::Variable::Health) <= 0;
}

void Entity::takeDamage(int32_t amount, Entity* attacker)
{
    if (amount <= 0 || isDead())
        return;

    // Check invulnerability (god mode / test characters)
    if (m_invulnerable)
        return;

    int32_t currentHealth = getVariable(ObjDefines::Variable::Health);
    int32_t newHealth = std::max(0, currentHealth - amount);
    bool inDuel = false;
    ::Player* victimPlayer = nullptr;

    if (::Player* attackerPlayer = dynamic_cast<::Player*>(attacker))
    {
        victimPlayer = dynamic_cast<::Player*>(this);
        if (victimPlayer && sDuelManager.areDueling(victimPlayer->getGuid(), attackerPlayer->getGuid()))
        {
            inDuel = true;
            if (newHealth <= Duel::DUEL_MIN_HEALTH)
            {
                newHealth = Duel::DUEL_MIN_HEALTH;
            }
        }
    }

    setVariable(ObjDefines::Variable::Health, newHealth);
    broadcastVariable(ObjDefines::Variable::Health, newHealth);

    LOG_DEBUG("Entity %lu: Took %d damage (health: %d -> %d)",
              getGuid(), amount, currentHealth, newHealth);

    if (inDuel && victimPlayer)
    {
        if (sDuelManager.onDuelDamage(victimPlayer, newHealth))
            return;
    }

    // Check for death
    if (newHealth <= 0 && currentHealth > 0)
    {
        onDeath(attacker);
    }
}

void Entity::heal(int32_t amount, Entity* healer)
{
    if (amount <= 0 || isDead())
        return;

    (void)healer;  // May be used for healing threat

    int32_t currentHealth = getVariable(ObjDefines::Variable::Health);
    int32_t maxHealth = getVariable(ObjDefines::Variable::MaxHealth);
    int32_t newHealth = std::min(maxHealth, currentHealth + amount);

    if (newHealth > currentHealth)
    {
        setVariable(ObjDefines::Variable::Health, newHealth);
        broadcastVariable(ObjDefines::Variable::Health, newHealth);

        LOG_DEBUG("Entity %lu: Healed for %d (health: %d -> %d)",
                  getGuid(), amount, currentHealth, newHealth);
    }
}

void Entity::onDeath(Entity* killer)
{
    LOG_INFO("Entity %lu: Died (killer=%lu)", getGuid(),
             killer ? killer->getGuid() : 0);

    // Set dead state
    setDead(true);

    // Clear non-persistent auras on death
    m_auras.clearAll(false);  // Keep persistent auras

    // Notify callback if set
    if (m_variableCallback)
    {
        m_variableCallback(this, ObjDefines::Variable::IsDead, 0, 1);
    }
}

void Entity::setDead(bool dead)
{
    int32_t deadValue = dead ? 1 : 0;
    setVariable(ObjDefines::Variable::IsDead, deadValue);
    broadcastVariable(ObjDefines::Variable::IsDead, deadValue);

    // If resurrecting, also update health variable
    if (!dead)
    {
        broadcastVariable(ObjDefines::Variable::Health, getVariable(ObjDefines::Variable::Health));
    }
}

void Entity::broadcastVariable(ObjDefines::Variable var, int32_t value)
{
    // NOTE: GP_Server_ObjectVariable is NOT supported by the client binary.
    // The client tracks Health/Mana changes internally from GP_Server_CombatMsg.
    // For critical state changes, we resend the full entity packet instead.

    (void)value;  // Value is already set via setVariable() before this call

    // Health/Mana updates: Client tracks these from CombatMsg damage/heal amounts
    // No need to broadcast - the client updates its internal state from combat messages
    if (var == ObjDefines::Variable::Health ||
        var == ObjDefines::Variable::Mana ||
        var == ObjDefines::Variable::MaxHealth ||
        var == ObjDefines::Variable::MaxMana)
    {
        // Skip - client handles these via GP_Server_CombatMsg
        return;
    }

    // For critical state changes (IsDead, dynamic flags, etc.),
    // resend the full entity packet to ensure client is in sync
    if (getType() == MutualObject::Type::Player)
    {
        ::Player* selfPlayer = static_cast<::Player*>(this);
        // Resend full player data to all viewers (includes updated variables)
        sWorldManager.broadcastPlayerUpdate(selfPlayer);
    }
    else if (getType() == MutualObject::Type::Npc)
    {
        ::Npc* selfNpc = static_cast<::Npc*>(this);
        // Resend full NPC data to all players on the map
        sWorldManager.broadcastNpcUpdate(selfNpc);
    }
}
