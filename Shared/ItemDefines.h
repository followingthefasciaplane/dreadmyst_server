#pragma once

#include <stdint.h>

namespace ItemDefines
{
    // Weapon types for combat sounds and animations
    enum class WeaponType : uint8_t
    {
        None = 0,
        Sword = 1,
        Axe = 2,
        Mace = 3,
        Dagger = 4,
        Staff = 5,
        Bow = 6,
        Crossbow = 7,
        Wand = 8,
        Gun = 9,
        Polearm = 10,
        Fist = 11,
        MaxWeaponType
    };

    // Equipment slot types (from game.db item_template.equip_type)
    enum class EquipType : uint8_t
    {
        None = 0,
        Helm = 1,
        Necklace = 2,
        Chest = 3,
        Belt = 4,
        Legs = 5,
        Feet = 6,
        Hands = 7,
        Ring = 8,
        Weapon = 9,
        Shield = 10,
        Ranged = 11,
        MaxEquipType
    };

    // Armor material types (from game.db item_template.armor_type)
    enum class ArmorType : uint8_t
    {
        None = 0,
        Cloth = 1,
        Leather = 2,
        Mail = 3,
        Plate = 4,
        MaxArmorType
    };

    // Weapon material types (from game.db item_template.weapon_material)
    enum class WeaponMaterial : uint8_t
    {
        None = 0,
        Wood = 1,
        Iron = 2,
        Steel = 3,
        Mithril = 4,
        Adamantine = 5,
        MaxWeaponMaterial
    };

    // Item quality/rarity
    enum class Quality : uint8_t
    {
        Poor = 0,       // Gray
        Common = 1,     // White
        Uncommon = 2,   // Green
        Rare = 3,       // Blue
        Epic = 4,       // Purple
        Legendary = 5,  // Orange
    };

    // Item flags (bitmask)
    namespace ItemFlags
    {
        constexpr int ItemFlag_Soulbound = 0x0001;
        constexpr int ItemFlag_QuestItem = 0x0002;
        constexpr int ItemFlag_Skillbook = 0x0004;
        constexpr int ItemFlag_Unique = 0x0008;
        constexpr int ItemFlag_Stackable = 0x0010;
        constexpr int ItemFlag_Consumable = 0x0020;
    }

    // Static item IDs - hardcoded items with special behavior
    namespace StaticItems
    {
        constexpr int GoldItem = 24;
    }

    // Item reference structure (used for inventory/equipment serialization)
    struct ItemId
    {
        int m_itemId = 0;   // Item template ID
        union
        {
            struct
            {
                int m_affix1;
                int m_affix2;
            };
            struct
            {
                int m_affixId;
                int m_affixScore;
            };
        };
        int m_gem1 = 0;     // Socket gem 1
        int m_gem2 = 0;     // Socket gem 2
        int m_gem3 = 0;     // Socket gem 3
        int m_gem4 = 0;     // Socket gem 4

        ItemId() { m_affix1 = 0; m_affix2 = 0; }
        ItemId(int id) : m_itemId(id) { m_affix1 = 0; m_affix2 = 0; }

        bool valid() const { return m_itemId > 0; }
        operator bool() const { return valid(); }
    };

    // Full item definition with additional properties
    struct ItemDefinition
    {
        int m_itemId = 0;       // Item template ID
        union
        {
            struct
            {
                int m_affix1;
                int m_affix2;
            };
            struct
            {
                int m_affixId;
                int m_affixScore;
            };
        };
        int m_gem1 = 0;         // Socket gem 1
        int m_gem2 = 0;         // Socket gem 2
        int m_gem3 = 0;         // Socket gem 3
        int m_gem4 = 0;         // Socket gem 4
        int m_count = 1;        // Stack count
        int m_durability = 100; // Current durability (percentage)
        union
        {
            int m_enchant;
            int m_enchantLvl;
        };
        bool m_soulbound = false; // Whether item is bound to player

        ItemDefinition() { m_affix1 = 0; m_affix2 = 0; m_enchant = 0; }
        ItemDefinition(int id) : m_itemId(id) { m_affix1 = 0; m_affix2 = 0; m_enchant = 0; }
        ItemDefinition(int id, int count) : m_itemId(id), m_count(count) { m_affix1 = 0; m_affix2 = 0; m_enchant = 0; }

        bool valid() const { return m_itemId > 0; }
        operator bool() const { return valid(); }

        // Implicit conversion to ItemId for serialization
        operator ItemId() const
        {
            ItemId id;
            id.m_itemId = m_itemId;
            id.m_affix1 = m_affix1;
            id.m_affix2 = m_affix2;
            id.m_gem1 = m_gem1;
            id.m_gem2 = m_gem2;
            id.m_gem3 = m_gem3;
            id.m_gem4 = m_gem4;
            return id;
        }
    };
}
