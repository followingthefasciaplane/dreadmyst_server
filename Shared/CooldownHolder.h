#pragma once

#include <map>
#include <ctime>
#include <cstdint>
#include <utility>

// __time64_t compatibility
#ifndef _WIN32
typedef int64_t __time64_t;
#endif

// Tracks spell cooldowns
class CooldownHolder
{
public:
    void setCooldown(int spellId, int durationMs)
    {
        m_cooldowns[spellId] = clock() + (durationMs * CLOCKS_PER_SEC / 1000);
    }

    bool isOnCooldown(int spellId) const
    {
        auto it = m_cooldowns.find(spellId);
        if (it == m_cooldowns.end()) return false;
        return clock() < it->second;
    }

    int getRemainingMs(int spellId) const
    {
        auto it = m_cooldowns.find(spellId);
        if (it == m_cooldowns.end()) return 0;
        clock_t remaining = it->second - clock();
        if (remaining <= 0) return 0;
        return static_cast<int>(remaining * 1000 / CLOCKS_PER_SEC);
    }

    void clearCooldown(int spellId) { m_cooldowns.erase(spellId); }
    void clearAll() { m_cooldowns.clear(); }

    // Update cooldowns - removes expired entries (called each frame)
    void updateCooldowns(__time64_t /*currentTimeMs*/)
    {
        // Remove expired cooldowns
        for (auto it = m_cooldowns.begin(); it != m_cooldowns.end(); )
        {
            if (clock() >= it->second)
            {
                m_cooldownStarts.erase(it->first);
                it = m_cooldowns.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    // Get cooldown as (startTime, endTime) pair for UI display
    std::pair<__time64_t, __time64_t> getCooldown(int spellId) const
    {
        auto it = m_cooldowns.find(spellId);
        if (it == m_cooldowns.end())
            return {0, 0};
        auto startIt = m_cooldownStarts.find(spellId);
        __time64_t start = (startIt != m_cooldownStarts.end()) ? startIt->second : 0;
        return {start, static_cast<__time64_t>(it->second)};
    }

    void setCooldown(int spellId, __time64_t startTime, __time64_t endTime)
    {
        m_cooldownStarts[spellId] = startTime;
        m_cooldowns[spellId] = static_cast<clock_t>(endTime);
    }

private:
    std::map<int, clock_t> m_cooldowns;  // spellId -> expiry time
    std::map<int, __time64_t> m_cooldownStarts;  // spellId -> start time
};
