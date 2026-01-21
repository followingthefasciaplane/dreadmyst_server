// ItemFunctions.h - Item utility functions for display strings
#pragma once

#include "ItemDefines.h"
#include <string>

namespace ItemFunctions
{
    // Quality display names
    inline std::string qualityName(ItemDefines::Quality quality)
    {
        switch (quality)
        {
            case ItemDefines::Quality::Poor:      return "Poor";
            case ItemDefines::Quality::Common:    return "Common";
            case ItemDefines::Quality::Uncommon:  return "Uncommon";
            case ItemDefines::Quality::Rare:      return "Rare";
            case ItemDefines::Quality::Epic:      return "Epic";
            case ItemDefines::Quality::Legendary: return "Legendary";
            default: return "";
        }
    }

    // Armor type display names
    inline std::string armorTypeName(ItemDefines::ArmorType type)
    {
        switch (type)
        {
            case ItemDefines::ArmorType::Cloth:   return "Cloth";
            case ItemDefines::ArmorType::Leather: return "Leather";
            case ItemDefines::ArmorType::Mail:    return "Mail";
            case ItemDefines::ArmorType::Plate:   return "Plate";
            default: return "";
        }
    }

    // Equipment type display names
    inline std::string equipTypeName(ItemDefines::EquipType type)
    {
        switch (type)
        {
            case ItemDefines::EquipType::Helm:    return "Helm";
            case ItemDefines::EquipType::Necklace: return "Necklace";
            case ItemDefines::EquipType::Chest:   return "Chest";
            case ItemDefines::EquipType::Belt:    return "Belt";
            case ItemDefines::EquipType::Legs:    return "Legs";
            case ItemDefines::EquipType::Feet:    return "Feet";
            case ItemDefines::EquipType::Hands:   return "Hands";
            case ItemDefines::EquipType::Ring:    return "Ring";
            case ItemDefines::EquipType::Weapon:  return "Weapon";
            case ItemDefines::EquipType::Shield:  return "Shield";
            case ItemDefines::EquipType::Ranged:  return "Ranged";
            default: return "";
        }
    }

    // Weapon type display names
    inline std::string weaponTypeName(ItemDefines::WeaponType type)
    {
        switch (type)
        {
            case ItemDefines::WeaponType::Sword:    return "Sword";
            case ItemDefines::WeaponType::Axe:      return "Axe";
            case ItemDefines::WeaponType::Mace:     return "Mace";
            case ItemDefines::WeaponType::Dagger:   return "Dagger";
            case ItemDefines::WeaponType::Staff:    return "Staff";
            case ItemDefines::WeaponType::Bow:      return "Bow";
            case ItemDefines::WeaponType::Crossbow: return "Crossbow";
            case ItemDefines::WeaponType::Wand:     return "Wand";
            case ItemDefines::WeaponType::Gun:      return "Gun";
            case ItemDefines::WeaponType::Polearm:  return "Polearm";
            case ItemDefines::WeaponType::Fist:     return "Fist";
            default: return "";
        }
    }

    // Weapon material type display names
    inline std::string weaponMaterialType(ItemDefines::WeaponMaterial material)
    {
        switch (material)
        {
            case ItemDefines::WeaponMaterial::Wood:       return "Wooden";
            case ItemDefines::WeaponMaterial::Iron:       return "Iron";
            case ItemDefines::WeaponMaterial::Steel:      return "Steel";
            case ItemDefines::WeaponMaterial::Mithril:    return "Mithril";
            case ItemDefines::WeaponMaterial::Adamantine: return "Adamantine";
            default: return "";
        }
    }

    // Affix score display names (quality tier based on affix score)
    inline std::string affixScoreName(int score)
    {
        if (score >= 5) return "Legendary";
        if (score >= 4) return "Epic";
        if (score >= 3) return "Superior";
        if (score >= 2) return "Fine";
        return "";
    }
}
