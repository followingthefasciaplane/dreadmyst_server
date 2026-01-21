#pragma once

#include <stdint.h>
#include <string>

namespace NpcDefines
{
    // NPC creature types for tooltips and AI
    enum class Type : uint8_t
    {
        None = 0,
        Beast = 1,
        Humanoid = 2,
        Undead = 3,
        Demon = 4,
        Elemental = 5,
        Dragon = 6,
        Giant = 7,
        Mechanical = 8,
        Aberration = 9,
        MaxType
    };

    // NPC default movement behavior types
    enum class DefaultMovement : uint8_t
    {
        None = 0,    // Stay in place
        Random = 1,  // Wander randomly within range
        Patrol = 2   // Follow waypoint path
    };

    // NPC flags (bitmask) - determines NPC interaction types
    namespace Flags
    {
        constexpr uint32_t None       = 0x0000;
        constexpr uint32_t Gossip     = 0x0001;
        constexpr uint32_t QuestGiver = 0x0002;
        constexpr uint32_t Vendor     = 0x0004;
        constexpr uint32_t Trainer    = 0x0008;
        constexpr uint32_t Innkeeper  = 0x0010;
        constexpr uint32_t FlightMaster = 0x0020;
        constexpr uint32_t Banker     = 0x0040;
        constexpr uint32_t Repair     = 0x0080;
        constexpr uint32_t Mailbox    = 0x0100;
        // Special hint/credit flags for UI indicators
        constexpr uint32_t SpendExpCredit = 0x0200;
        constexpr uint32_t LevelToCredit  = 0x0400;
        constexpr uint32_t TalkCredit     = 0x0800;
    }
}

namespace NpcFunctions
{
    inline std::string typeName(NpcDefines::Type type)
    {
        switch (type)
        {
            case NpcDefines::Type::Beast:      return "Beast";
            case NpcDefines::Type::Humanoid:   return "Humanoid";
            case NpcDefines::Type::Undead:     return "Undead";
            case NpcDefines::Type::Demon:      return "Demon";
            case NpcDefines::Type::Elemental:  return "Elemental";
            case NpcDefines::Type::Dragon:     return "Dragon";
            case NpcDefines::Type::Giant:      return "Giant";
            case NpcDefines::Type::Mechanical: return "Mechanical";
            case NpcDefines::Type::Aberration: return "Aberration";
            default:                           return "";
        }
    }
}
