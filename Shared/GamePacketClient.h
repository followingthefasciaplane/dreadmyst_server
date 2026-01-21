#pragma once

#include "GamePacketBase.h"
#include "ItemDefines.h"
#include "UnitDefines.h"
#include <map>
#include <string>

// Use the helper functions from GamePacketServer.h
// (they're inline so can be used here too)
inline void packItemIdClient(StlBuffer& buf, const ItemDefines::ItemId& item)
{
    buf << item.m_itemId << item.m_affix1 << item.m_affix2
        << item.m_gem1 << item.m_gem2 << item.m_gem3;
}

inline void unpackItemIdClient(StlBuffer& buf, ItemDefines::ItemId& item)
{
    buf >> item.m_itemId >> item.m_affix1 >> item.m_affix2
        >> item.m_gem1 >> item.m_gem2 >> item.m_gem3;
}

// Helper for map serialization (mirrors GamePacketServer.h)
template<typename K, typename V>
inline void packMapClient(StlBuffer& buf, const std::map<K, V>& m)
{
    buf << static_cast<uint16_t>(m.size());
    for (const auto& [key, val] : m) {
        buf << key << val;
    }
}

template<typename K, typename V>
inline void unpackMapClient(StlBuffer& buf, std::map<K, V>& m)
{
    uint16_t count;
    buf >> count;
    m.clear();
    for (uint16_t i = 0; i < count; ++i) {
        K key;
        V val;
        buf >> key >> val;
        m[key] = val;
    }
}

// ============================================
// Client -> Server Packets
// ============================================

struct GP_Client_Authenticate : public GamePacket
{
    std::string m_token;
    int32_t m_buildVersion = 0;
    std::string m_fingerprint;

    uint16_t getOpcode() const override { return Opcode::Client_Authenticate; }
    void pack(StlBuffer& buf) const override { buf << m_token << m_buildVersion << m_fingerprint; }
    void unpack(StlBuffer& buf) override { buf >> m_token >> m_buildVersion >> m_fingerprint; }
};

struct GP_Client_CharacterList : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_CharacterList; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_CharCreate : public GamePacket
{
    std::string m_name;
    uint8_t m_classId = 0;
    uint8_t m_gender = 0;
    int32_t m_portraitId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_CharCreate; }
    void pack(StlBuffer& buf) const override { buf << m_name << m_classId << m_gender << m_portraitId; }
    void unpack(StlBuffer& buf) override { buf >> m_name >> m_classId >> m_gender >> m_portraitId; }
};

struct GP_Client_DeleteCharacter : public GamePacket
{
    uint32_t m_guid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_DeleteCharacter; }
    void pack(StlBuffer& buf) const override { buf << m_guid; }
    void unpack(StlBuffer& buf) override { buf >> m_guid; }
};

struct GP_Client_EnterWorld : public GamePacket
{
    uint32_t m_characterGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_EnterWorld; }
    void pack(StlBuffer& buf) const override { buf << m_characterGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_characterGuid; }
};

struct GP_Client_RequestMove : public GamePacket
{
    uint8_t m_wasdFlags = 0;  // WASD bitmask
    float m_destX = 0, m_destY = 0;

    uint16_t getOpcode() const override { return Opcode::Client_RequestMove; }
    void pack(StlBuffer& buf) const override { buf << m_wasdFlags << m_destX << m_destY; }
    void unpack(StlBuffer& buf) override { buf >> m_wasdFlags >> m_destX >> m_destY; }
};

struct GP_Client_RequestStop : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_RequestStop; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_CastSpell : public GamePacket
{
    int32_t m_spellId = 0;
    uint32_t m_targetGuid = 0;
    float m_targetX = 0, m_targetY = 0;  // For ground-targeted spells

    uint16_t getOpcode() const override { return Opcode::Client_CastSpell; }
    void pack(StlBuffer& buf) const override { buf << m_spellId << m_targetGuid << m_targetX << m_targetY; }
    void unpack(StlBuffer& buf) override { buf >> m_spellId >> m_targetGuid >> m_targetX >> m_targetY; }
};

struct GP_Client_CancelCast : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_CancelCast; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_CancelBuff : public GamePacket
{
    int32_t m_spellId = 0;
    uint32_t m_casterGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_CancelBuff; }
    void pack(StlBuffer& buf) const override { buf << m_spellId << m_casterGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_spellId >> m_casterGuid; }
};

struct GP_Client_SetSelected : public GamePacket
{
    uint32_t m_guid = 0;  // 0 to deselect

    uint16_t getOpcode() const override { return Opcode::Client_SetSelected; }
    void pack(StlBuffer& buf) const override { buf << m_guid; }
    void unpack(StlBuffer& buf) override { buf >> m_guid; }
};

struct GP_Client_EquipItem : public GamePacket
{
    int32_t m_slotInv = 0;              // Inventory slot to take item from
    UnitDefines::EquipSlot m_slotEquip = UnitDefines::EquipSlot::None;  // Target equipment slot (optional)
    ItemDefines::ItemId m_itemId;       // Item being equipped

    uint16_t getOpcode() const override { return Opcode::Client_EquipItem; }
    void pack(StlBuffer& buf) const override { buf << m_slotInv << static_cast<uint8_t>(m_slotEquip); packItemIdClient(buf, m_itemId); }
    void unpack(StlBuffer& buf) override { uint8_t slot; buf >> m_slotInv >> slot; m_slotEquip = static_cast<UnitDefines::EquipSlot>(slot); unpackItemIdClient(buf, m_itemId); }
};

struct GP_Client_UnequipItem : public GamePacket
{
    int32_t m_equipSlot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_UnequipItem; }
    void pack(StlBuffer& buf) const override { buf << m_equipSlot; }
    void unpack(StlBuffer& buf) override { buf >> m_equipSlot; }
};

struct GP_Client_MoveItem : public GamePacket
{
    int32_t m_fromSlot = 0;
    int32_t m_toSlot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_MoveItem; }
    void pack(StlBuffer& buf) const override { buf << m_fromSlot << m_toSlot; }
    void unpack(StlBuffer& buf) override { buf >> m_fromSlot >> m_toSlot; }
};

struct GP_Client_SplitItemStack : public GamePacket
{
    int32_t m_fromSlot = 0;
    int32_t m_toSlot = 0;
    int32_t m_amount = 0;

    uint16_t getOpcode() const override { return Opcode::Client_SplitItemStack; }
    void pack(StlBuffer& buf) const override { buf << m_fromSlot << m_toSlot << m_amount; }
    void unpack(StlBuffer& buf) override { buf >> m_fromSlot >> m_toSlot >> m_amount; }
};

struct GP_Client_DestroyItem : public GamePacket
{
    int32_t m_slot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_DestroyItem; }
    void pack(StlBuffer& buf) const override { buf << m_slot; }
    void unpack(StlBuffer& buf) override { buf >> m_slot; }
};

struct GP_Client_UseItem : public GamePacket
{
    int32_t m_slot = 0;
    uint32_t m_targetGuid = 0;
    ItemDefines::ItemId m_itemId;  // Item being used
    UnitDefines::EquipSlot m_target_EquipSlot = UnitDefines::EquipSlot::None;  // Target equipment slot (for item-on-equipment use)

    uint16_t getOpcode() const override { return Opcode::Client_UseItem; }
    void pack(StlBuffer& buf) const override { buf << m_slot << m_targetGuid; packItemIdClient(buf, m_itemId); buf << static_cast<uint8_t>(m_target_EquipSlot); }
    void unpack(StlBuffer& buf) override { buf >> m_slot >> m_targetGuid; unpackItemIdClient(buf, m_itemId); uint8_t slot; buf >> slot; m_target_EquipSlot = static_cast<UnitDefines::EquipSlot>(slot); }
};

struct GP_Client_SortInventory : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_SortInventory; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_ReqAbilityList : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_ReqAbilityList; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_LootItem : public GamePacket
{
    static constexpr uint16_t TakeAll = 0xFFFF;  // Special value for "take all items"

    uint32_t m_sourceGuid = 0;  // GUID of object being looted
    ItemDefines::ItemId m_itemId;  // Item to loot (or TakeAll)

    uint16_t getOpcode() const override { return Opcode::Client_LootItem; }
    void pack(StlBuffer& buf) const override { buf << m_sourceGuid; packItemIdClient(buf, m_itemId); }
    void unpack(StlBuffer& buf) override { buf >> m_sourceGuid; unpackItemIdClient(buf, m_itemId); }
};

struct GP_Client_OpenBank : public GamePacket
{
    uint32_t m_bankerGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_OpenBank; }
    void pack(StlBuffer& buf) const override { buf << m_bankerGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_bankerGuid; }
};

struct GP_Client_MoveInventoryToBank : public GamePacket
{
    int32_t m_from = 0;             // Inventory slot
    int32_t m_to = 0;               // Bank slot (ignored if m_autoSelectForMe)
    bool m_autoSelectForMe = false; // Server picks first empty bank slot

    uint16_t getOpcode() const override { return Opcode::Client_MoveInventoryToBank; }
    void pack(StlBuffer& buf) const override { buf << m_from << m_to << m_autoSelectForMe; }
    void unpack(StlBuffer& buf) override { buf >> m_from >> m_to >> m_autoSelectForMe; }
};

struct GP_Client_MoveBankToBank : public GamePacket
{
    int32_t m_from = 0;   // Source bank slot
    int32_t m_to = 0;     // Destination bank slot

    uint16_t getOpcode() const override { return Opcode::Client_MoveBankToBank; }
    void pack(StlBuffer& buf) const override { buf << m_from << m_to; }
    void unpack(StlBuffer& buf) override { buf >> m_from >> m_to; }
};

struct GP_Client_UnBankItem : public GamePacket
{
    int32_t m_slot = 0;                 // Bank slot to withdraw from
    int32_t m_inventorySlot = 0;        // Inventory slot (ignored if m_chooseInvSlotForMe)
    bool m_chooseInvSlotForMe = false;  // Server picks first empty inventory slot

    uint16_t getOpcode() const override { return Opcode::Client_UnBankItem; }
    void pack(StlBuffer& buf) const override { buf << m_slot << m_inventorySlot << m_chooseInvSlotForMe; }
    void unpack(StlBuffer& buf) override { buf >> m_slot >> m_inventorySlot >> m_chooseInvSlotForMe; }
};

struct GP_Client_SortBank : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_SortBank; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_BuyVendorItem : public GamePacket
{
    uint32_t m_vendorGuid = 0;
    int32_t m_itemIndex = 0;
    int32_t m_count = 1;

    uint16_t getOpcode() const override { return Opcode::Client_BuyVendorItem; }
    void pack(StlBuffer& buf) const override { buf << m_vendorGuid << m_itemIndex << m_count; }
    void unpack(StlBuffer& buf) override { buf >> m_vendorGuid >> m_itemIndex >> m_count; }
};

struct GP_Client_SellItem : public GamePacket
{
    uint32_t m_vendorGuid = 0;
    int32_t m_inventorySlot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_SellItem; }
    void pack(StlBuffer& buf) const override { buf << m_vendorGuid << m_inventorySlot; }
    void unpack(StlBuffer& buf) override { buf >> m_vendorGuid >> m_inventorySlot; }
};

struct GP_Client_Buyback : public GamePacket
{
    uint32_t m_vendorGuid = 0;
    int32_t m_buybackSlot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_Buyback; }
    void pack(StlBuffer& buf) const override { buf << m_vendorGuid << m_buybackSlot; }
    void unpack(StlBuffer& buf) override { buf >> m_vendorGuid >> m_buybackSlot; }
};

struct GP_Client_OpenTradeWith : public GamePacket
{
    uint32_t m_targetGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_OpenTradeWith; }
    void pack(StlBuffer& buf) const override { buf << m_targetGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_targetGuid; }
};

struct GP_Client_TradeAddItem : public GamePacket
{
    int32_t m_invSlot = 0;  // Inventory slot of item to add to trade

    uint16_t getOpcode() const override { return Opcode::Client_TradeAddItem; }
    void pack(StlBuffer& buf) const override { buf << m_invSlot; }
    void unpack(StlBuffer& buf) override { buf >> m_invSlot; }
};

struct GP_Client_TradeRemoveItem : public GamePacket
{
    int32_t m_itemGuid = 0;  // Item GUID to remove from trade (not slot-based)

    uint16_t getOpcode() const override { return Opcode::Client_TradeRemoveItem; }
    void pack(StlBuffer& buf) const override { buf << m_itemGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_itemGuid; }
};

struct GP_Client_TradeSetGold : public GamePacket
{
    int32_t m_amount = 0;  // Gold amount to offer in trade

    uint16_t getOpcode() const override { return Opcode::Client_TradeSetGold; }
    void pack(StlBuffer& buf) const override { buf << m_amount; }
    void unpack(StlBuffer& buf) override { buf >> m_amount; }
};

struct GP_Client_TradeConfirm : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_TradeConfirm; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_TradeCancel : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_TradeCancel; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_Repair : public GamePacket
{
    bool m_confirmed = false;

    uint16_t getOpcode() const override { return Opcode::Client_Repair; }
    void pack(StlBuffer& buf) const override { buf << m_confirmed; }
    void unpack(StlBuffer& buf) override { buf >> m_confirmed; }
};

struct GP_Client_AcceptQuest : public GamePacket
{
    uint32_t m_questGiverGuid = 0;
    int32_t m_questId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_AcceptQuest; }
    void pack(StlBuffer& buf) const override { buf << m_questGiverGuid << m_questId; }
    void unpack(StlBuffer& buf) override { buf >> m_questGiverGuid >> m_questId; }
};

struct GP_Client_CompleteQuest : public GamePacket
{
    uint32_t m_questGiverGuid = 0;
    int32_t m_questId = 0;
    int32_t m_itemChoiceIdx = 0;

    uint16_t getOpcode() const override { return Opcode::Client_CompleteQuest; }
    void pack(StlBuffer& buf) const override { buf << m_questGiverGuid << m_questId << m_itemChoiceIdx; }
    void unpack(StlBuffer& buf) override { buf >> m_questGiverGuid >> m_questId >> m_itemChoiceIdx; }
};

struct GP_Client_AbandonQuest : public GamePacket
{
    int32_t m_questId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_AbandonQuest; }
    void pack(StlBuffer& buf) const override { buf << m_questId; }
    void unpack(StlBuffer& buf) override { buf >> m_questId; }
};

struct GP_Client_ClickedGossipOption : public GamePacket
{
    int32_t m_entry = 0;

    uint16_t getOpcode() const override { return Opcode::Client_ClickedGossipOption; }
    void pack(StlBuffer& buf) const override { buf << m_entry; }
    void unpack(StlBuffer& buf) override { buf >> m_entry; }
};

struct GP_Client_ChatMsg : public GamePacket
{
    uint8_t m_channelId = 0;
    std::string m_text;
    std::string m_targetName;  // For whispers
    ItemDefines::ItemDefinition m_itemId;  // Linked item (if any)

    uint16_t getOpcode() const override { return Opcode::Client_ChatMsg; }
    void pack(StlBuffer& buf) const override {
        buf << m_channelId << m_text << m_targetName;
        buf << m_itemId.m_itemId << m_itemId.m_affix1 << m_itemId.m_affix2
            << m_itemId.m_gem1 << m_itemId.m_gem2 << m_itemId.m_gem3;
    }
    void unpack(StlBuffer& buf) override {
        buf >> m_channelId >> m_text >> m_targetName;
        buf >> m_itemId.m_itemId >> m_itemId.m_affix1 >> m_itemId.m_affix2
            >> m_itemId.m_gem1 >> m_itemId.m_gem2 >> m_itemId.m_gem3;
    }
};

struct GP_Client_SetIgnorePlayer : public GamePacket
{
    std::string m_playerName;
    bool m_ignore = true;

    uint16_t getOpcode() const override { return Opcode::Client_SetIgnorePlayer; }
    void pack(StlBuffer& buf) const override { buf << m_playerName << m_ignore; }
    void unpack(StlBuffer& buf) override { buf >> m_playerName >> m_ignore; }
};

struct GP_Client_GuildCreate : public GamePacket
{
    std::string m_guildName;

    uint16_t getOpcode() const override { return Opcode::Client_GuildCreate; }
    void pack(StlBuffer& buf) const override { buf << m_guildName; }
    void unpack(StlBuffer& buf) override { buf >> m_guildName; }
};

struct GP_Client_GuildInviteMember : public GamePacket
{
    std::string m_playerName;

    uint16_t getOpcode() const override { return Opcode::Client_GuildInviteMember; }
    void pack(StlBuffer& buf) const override { buf << m_playerName; }
    void unpack(StlBuffer& buf) override { buf >> m_playerName; }
};

struct GP_Client_GuildInviteResponse : public GamePacket
{
    int32_t m_guildId = 0;
    bool m_accept = false;

    uint16_t getOpcode() const override { return Opcode::Client_GuildInviteResponse; }
    void pack(StlBuffer& buf) const override { buf << m_guildId << m_accept; }
    void unpack(StlBuffer& buf) override { buf >> m_guildId >> m_accept; }
};

struct GP_Client_GuildQuit : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_GuildQuit; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_GuildKickMember : public GamePacket
{
    uint32_t m_memberGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_GuildKickMember; }
    void pack(StlBuffer& buf) const override { buf << m_memberGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_memberGuid; }
};

struct GP_Client_GuildPromoteMember : public GamePacket
{
    uint32_t m_memberGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_GuildPromoteMember; }
    void pack(StlBuffer& buf) const override { buf << m_memberGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_memberGuid; }
};

struct GP_Client_GuildDemoteMember : public GamePacket
{
    uint32_t m_memberGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_GuildDemoteMember; }
    void pack(StlBuffer& buf) const override { buf << m_memberGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_memberGuid; }
};

struct GP_Client_GuildDisband : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_GuildDisband; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_GuildMotd : public GamePacket
{
    std::string m_motd;

    uint16_t getOpcode() const override { return Opcode::Client_GuildMotd; }
    void pack(StlBuffer& buf) const override { buf << m_motd; }
    void unpack(StlBuffer& buf) override { buf >> m_motd; }
};

struct GP_Client_GuildRosterRequest : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_GuildRosterRequest; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_PartyInviteMember : public GamePacket
{
    std::string m_playerName;

    uint16_t getOpcode() const override { return Opcode::Client_PartyInviteMember; }
    void pack(StlBuffer& buf) const override { buf << m_playerName; }
    void unpack(StlBuffer& buf) override { buf >> m_playerName; }
};

struct GP_Client_PartyInviteResponse : public GamePacket
{
    bool m_accept = false;

    uint16_t getOpcode() const override { return Opcode::Client_PartyInviteResponse; }
    void pack(StlBuffer& buf) const override { buf << m_accept; }
    void unpack(StlBuffer& buf) override { buf >> m_accept; }
};

struct GP_Client_PartyChanges : public GamePacket
{
    uint8_t m_changeType = 0;  // Leave, kick, promote, etc.
    uint32_t m_targetGuid = 0;

    uint16_t getOpcode() const override { return Opcode::Client_PartyChanges; }
    void pack(StlBuffer& buf) const override { buf << m_changeType << m_targetGuid; }
    void unpack(StlBuffer& buf) override { buf >> m_changeType >> m_targetGuid; }
};

struct GP_Client_DuelResponse : public GamePacket
{
    bool m_accept = false;

    uint16_t getOpcode() const override { return Opcode::Client_DuelResponse; }
    void pack(StlBuffer& buf) const override { buf << m_accept; }
    void unpack(StlBuffer& buf) override { buf >> m_accept; }
};

struct GP_Client_YieldDuel : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_YieldDuel; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_TogglePvP : public GamePacket
{
    bool m_enabled = false;

    uint16_t getOpcode() const override { return Opcode::Client_TogglePvP; }
    void pack(StlBuffer& buf) const override { buf << m_enabled; }
    void unpack(StlBuffer& buf) override { buf >> m_enabled; }
};

struct GP_Client_UpdateArenaStatus : public GamePacket
{
    bool m_enterArena = false;

    uint16_t getOpcode() const override { return Opcode::Client_UpdateArenaStatus; }
    void pack(StlBuffer& buf) const override { buf << m_enterArena; }
    void unpack(StlBuffer& buf) override { buf >> m_enterArena; }
};

struct GP_Client_Respec : public GamePacket
{
    bool m_confirmed = false;

    uint16_t getOpcode() const override { return Opcode::Client_Respec; }
    void pack(StlBuffer& buf) const override { buf << m_confirmed; }
    void unpack(StlBuffer& buf) override { buf >> m_confirmed; }
};

struct GP_Client_LevelUp : public GamePacket
{
    std::map<int32_t, int32_t> m_spellInvestments;  // spellId -> points
    std::map<int32_t, int32_t> m_statInvestments;   // statId -> points

    uint16_t getOpcode() const override { return Opcode::Client_LevelUp; }
    void pack(StlBuffer& buf) const override
    {
        packMapClient(buf, m_spellInvestments);
        packMapClient(buf, m_statInvestments);
    }
    void unpack(StlBuffer& buf) override
    {
        unpackMapClient(buf, m_spellInvestments);
        unpackMapClient(buf, m_statInvestments);
    }
};

struct GP_Client_QueryWaypoints : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_QueryWaypoints; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_ActivateWaypoint : public GamePacket
{
    int32_t m_waypointId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_ActivateWaypoint; }
    void pack(StlBuffer& buf) const override { buf << m_waypointId; }
    void unpack(StlBuffer& buf) override { buf >> m_waypointId; }
};

struct GP_Client_RequestRespawn : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_RequestRespawn; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_ResetDungeons : public GamePacket
{
    uint16_t getOpcode() const override { return Opcode::Client_ResetDungeons; }
    void pack(StlBuffer& buf) const override { (void)buf; }
    void unpack(StlBuffer& buf) override { (void)buf; }
};

struct GP_Client_SocketItem : public GamePacket
{
    int32_t m_targetSlot = 0;
    int32_t m_gemSlot = 0;
    int32_t m_socketIndex = 0;

    uint16_t getOpcode() const override { return Opcode::Client_SocketItem; }
    void pack(StlBuffer& buf) const override { buf << m_targetSlot << m_gemSlot << m_socketIndex; }
    void unpack(StlBuffer& buf) override { buf >> m_targetSlot >> m_gemSlot >> m_socketIndex; }
};

struct GP_Client_EmpowerItem : public GamePacket
{
    int32_t m_targetSlot = 0;
    int32_t m_materialSlot = 0;

    uint16_t getOpcode() const override { return Opcode::Client_EmpowerItem; }
    void pack(StlBuffer& buf) const override { buf << m_targetSlot << m_materialSlot; }
    void unpack(StlBuffer& buf) override { buf >> m_targetSlot >> m_materialSlot; }
};

struct GP_Client_RollDice : public GamePacket
{
    int32_t m_maxValue = 100;

    uint16_t getOpcode() const override { return Opcode::Client_RollDice; }
    void pack(StlBuffer& buf) const override { buf << m_maxValue; }
    void unpack(StlBuffer& buf) override { buf >> m_maxValue; }
};

struct GP_Client_ReportPlayer : public GamePacket
{
    uint32_t m_targetGuid = 0;
    uint8_t m_reason = 0;
    std::string m_description;
    std::string m_playerName;  // Report by name (server looks up GUID)

    uint16_t getOpcode() const override { return Opcode::Client_ReportPlayer; }
    void pack(StlBuffer& buf) const override { buf << m_targetGuid << m_reason << m_description << m_playerName; }
    void unpack(StlBuffer& buf) override { buf >> m_targetGuid >> m_reason >> m_description >> m_playerName; }
};

struct GP_Client_MOD : public GamePacket
{
    std::string m_command;

    uint16_t getOpcode() const override { return Opcode::Client_MOD; }
    void pack(StlBuffer& buf) const override { buf << m_command; }
    void unpack(StlBuffer& buf) override { buf >> m_command; }
};

struct GP_Client_RecoverMailLoot : public GamePacket
{
    int32_t m_mailId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_RecoverMailLoot; }
    void pack(StlBuffer& buf) const override { buf << m_mailId; }
    void unpack(StlBuffer& buf) override { buf >> m_mailId; }
};

struct GP_Client_ReqTheoreticalSpell : public GamePacket
{
    int32_t m_spellId = 0;
    int32_t m_level = 0;

    uint16_t getOpcode() const override { return Opcode::Client_ReqTheoreticalSpell; }
    void pack(StlBuffer& buf) const override { buf << m_spellId << m_level; }
    void unpack(StlBuffer& buf) override { buf >> m_spellId >> m_level; }
};

struct GP_Client_SetToolbarSlot : public GamePacket
{
    int32_t m_slotIndex = 0;
    int32_t m_spellId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_SetToolbarSlot; }
    void pack(StlBuffer& buf) const override { buf << m_slotIndex << m_spellId; }
    void unpack(StlBuffer& buf) override { buf >> m_slotIndex >> m_spellId; }
};

struct GP_Client_ChangeChannels : public GamePacket
{
    uint32_t m_channelId = 0;

    uint16_t getOpcode() const override { return Opcode::Client_ChangeChannels; }
    void pack(StlBuffer& buf) const override { buf << m_channelId; }
    void unpack(StlBuffer& buf) override { buf >> m_channelId; }
};
