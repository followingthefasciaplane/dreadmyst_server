#pragma once

#include "StlBuffer.h"
#include <stdint.h>

// Packet opcodes
namespace Opcode
{
    // Bidirectional
    constexpr uint16_t Mutual_Ping = 0;

    // Server -> Client (100+)
    constexpr uint16_t Server_Validate = 100;
    constexpr uint16_t Server_QueuePosition = 101;
    constexpr uint16_t Server_CharacterList = 102;
    constexpr uint16_t Server_CharaCreateResult = 103;
    constexpr uint16_t Server_NewWorld = 104;
    constexpr uint16_t Server_SetController = 105;
    constexpr uint16_t Server_ChannelInfo = 106;
    constexpr uint16_t Server_ChannelChangeConfirm = 107;
    constexpr uint16_t Server_Player = 110;
    constexpr uint16_t Server_Npc = 111;
    constexpr uint16_t Server_GameObject = 112;
    constexpr uint16_t Server_DestroyObject = 113;
    constexpr uint16_t Server_SetSubname = 114;
    constexpr uint16_t Server_UnitSpline = 120;
    constexpr uint16_t Server_UnitTeleport = 121;
    constexpr uint16_t Server_UnitOrientation = 122;
    constexpr uint16_t Server_CastStart = 130;
    constexpr uint16_t Server_CastStop = 131;
    constexpr uint16_t Server_SpellGo = 132;
    constexpr uint16_t Server_CombatMsg = 133;
    constexpr uint16_t Server_UnitAuras = 134;
    constexpr uint16_t Server_Cooldown = 135;
    constexpr uint16_t Server_ObjectVariable = 136;
    constexpr uint16_t Server_AggroMob = 137;
    constexpr uint16_t Server_Inventory = 140;
    constexpr uint16_t Server_Bank = 141;
    constexpr uint16_t Server_OpenBank = 142;
    constexpr uint16_t Server_EquipItem = 143;
    constexpr uint16_t Server_NotifyItemAdd = 144;
    constexpr uint16_t Server_OpenLootWindow = 145;
    constexpr uint16_t Server_OnObjectWasLooted = 146;
    constexpr uint16_t Server_UpdateVendorStock = 147;
    constexpr uint16_t Server_RepairCost = 148;
    constexpr uint16_t Server_SocketResult = 149;
    constexpr uint16_t Server_EmpowerResult = 150;
    constexpr uint16_t Server_Spellbook = 160;
    constexpr uint16_t Server_Spellbook_Update = 161;
    constexpr uint16_t Server_LearnedSpell = 162;
    constexpr uint16_t Server_ExpNotify = 170;
    constexpr uint16_t Server_LvlResponse = 171;
    constexpr uint16_t Server_SpentGold = 172;
    constexpr uint16_t Server_QuestList = 180;
    constexpr uint16_t Server_AcceptedQuest = 181;
    constexpr uint16_t Server_QuestTally = 182;
    constexpr uint16_t Server_QuestComplete = 183;
    constexpr uint16_t Server_RewardedQuest = 184;
    constexpr uint16_t Server_AbandonQuest = 185;
    constexpr uint16_t Server_AvailableWorldQuests = 186;
    constexpr uint16_t Server_ChatMsg = 190;
    constexpr uint16_t Server_ChatError = 191;
    constexpr uint16_t Server_GossipMenu = 192;
    constexpr uint16_t Server_GuildRoster = 200;
    constexpr uint16_t Server_GuildInvite = 201;
    constexpr uint16_t Server_GuildAddMember = 202;
    constexpr uint16_t Server_GuildRemoveMember = 203;
    constexpr uint16_t Server_GuildOnlineStatus = 204;
    constexpr uint16_t Server_GuildNotifyRoleChange = 205;
    constexpr uint16_t Server_PartyList = 210;
    constexpr uint16_t Server_OfferParty = 211;
    constexpr uint16_t Server_TradeUpdate = 220;
    constexpr uint16_t Server_TradeCanceled = 221;
    constexpr uint16_t Server_QueryWaypointsResponse = 230;
    constexpr uint16_t Server_DiscoverWaypointNotify = 231;
    constexpr uint16_t Server_ArenaQueued = 240;
    constexpr uint16_t Server_ArenaReady = 241;
    constexpr uint16_t Server_ArenaStatus = 242;
    constexpr uint16_t Server_ArenaOutcome = 243;
    constexpr uint16_t Server_OfferDuel = 244;
    constexpr uint16_t Server_PkNotify = 245;
    constexpr uint16_t Server_WorldError = 250;
    constexpr uint16_t Server_InspectReveal = 251;
    constexpr uint16_t Server_PromptRespec = 252;
    constexpr uint16_t Server_RespawnResponse = 253;
    constexpr uint16_t Server_UnlockGameObj = 254;
    constexpr uint16_t Server_MarkNpcsOnMap = 255;

    // Client -> Server (300+)
    constexpr uint16_t Client_Authenticate = 300;
    constexpr uint16_t Client_CharacterList = 301;
    constexpr uint16_t Client_CharCreate = 302;
    constexpr uint16_t Client_DeleteCharacter = 303;
    constexpr uint16_t Client_EnterWorld = 304;
    constexpr uint16_t Client_RequestMove = 310;
    constexpr uint16_t Client_RequestStop = 311;
    constexpr uint16_t Client_CastSpell = 320;
    constexpr uint16_t Client_CancelCast = 321;
    constexpr uint16_t Client_SetSelected = 322;
    constexpr uint16_t Client_CancelBuff = 323;
    constexpr uint16_t Client_EquipItem = 330;
    constexpr uint16_t Client_UnequipItem = 331;
    constexpr uint16_t Client_MoveItem = 332;
    constexpr uint16_t Client_SplitItemStack = 333;
    constexpr uint16_t Client_DestroyItem = 334;
    constexpr uint16_t Client_UseItem = 335;
    constexpr uint16_t Client_SortInventory = 336;
    constexpr uint16_t Client_ReqAbilityList = 337;
    constexpr uint16_t Client_LootItem = 338;
    constexpr uint16_t Client_OpenBank = 340;
    constexpr uint16_t Client_MoveInventoryToBank = 341;
    constexpr uint16_t Client_MoveBankToBank = 342;
    constexpr uint16_t Client_UnBankItem = 343;
    constexpr uint16_t Client_SortBank = 344;
    constexpr uint16_t Client_BuyVendorItem = 350;
    constexpr uint16_t Client_SellItem = 351;
    constexpr uint16_t Client_Buyback = 352;
    constexpr uint16_t Client_OpenTradeWith = 353;
    constexpr uint16_t Client_TradeAddItem = 354;
    constexpr uint16_t Client_TradeRemoveItem = 355;
    constexpr uint16_t Client_TradeSetGold = 356;
    constexpr uint16_t Client_TradeConfirm = 357;
    constexpr uint16_t Client_TradeCancel = 358;
    constexpr uint16_t Client_Repair = 359;
    constexpr uint16_t Client_AcceptQuest = 360;
    constexpr uint16_t Client_CompleteQuest = 361;
    constexpr uint16_t Client_AbandonQuest = 362;
    constexpr uint16_t Client_ClickedGossipOption = 363;
    constexpr uint16_t Client_ChatMsg = 370;
    constexpr uint16_t Client_SetIgnorePlayer = 371;
    constexpr uint16_t Client_GuildCreate = 380;
    constexpr uint16_t Client_GuildInviteMember = 381;
    constexpr uint16_t Client_GuildInviteResponse = 382;
    constexpr uint16_t Client_GuildQuit = 383;
    constexpr uint16_t Client_GuildKickMember = 384;
    constexpr uint16_t Client_GuildPromoteMember = 385;
    constexpr uint16_t Client_GuildDemoteMember = 386;
    constexpr uint16_t Client_GuildDisband = 387;
    constexpr uint16_t Client_GuildMotd = 388;
    constexpr uint16_t Client_GuildRosterRequest = 389;
    constexpr uint16_t Client_PartyInviteMember = 390;
    constexpr uint16_t Client_PartyInviteResponse = 391;
    constexpr uint16_t Client_PartyChanges = 392;
    constexpr uint16_t Client_DuelResponse = 400;
    constexpr uint16_t Client_YieldDuel = 401;
    constexpr uint16_t Client_TogglePvP = 402;
    constexpr uint16_t Client_UpdateArenaStatus = 403;
    constexpr uint16_t Client_Respec = 410;
    constexpr uint16_t Client_LevelUp = 411;
    constexpr uint16_t Client_QueryWaypoints = 412;
    constexpr uint16_t Client_ActivateWaypoint = 413;
    constexpr uint16_t Client_RequestRespawn = 414;
    constexpr uint16_t Client_ResetDungeons = 415;
    constexpr uint16_t Client_SocketItem = 416;
    constexpr uint16_t Client_EmpowerItem = 417;
    constexpr uint16_t Client_RollDice = 418;
    constexpr uint16_t Client_ReportPlayer = 419;
    constexpr uint16_t Client_MOD = 420;
    constexpr uint16_t Client_RecoverMailLoot = 421;
    constexpr uint16_t Client_ReqTheoreticalSpell = 422;
    constexpr uint16_t Client_SetToolbarSlot = 423;
    constexpr uint16_t Client_ChangeChannels = 424;

    constexpr uint16_t MaxOpcode = 500;
}

// Base packet interface
class GamePacket
{
public:
    virtual ~GamePacket() = default;
    virtual uint16_t getOpcode() const = 0;
    virtual void pack(StlBuffer& buf) const = 0;
    virtual void unpack(StlBuffer& buf) = 0;

    // Build packet with opcode header
    StlBuffer build(StlBuffer buf) const
    {
        (void)buf;
        StlBuffer result;
        uint16_t op = getOpcode();
        result << op;
        pack(result);
        return result;
    }

    static bool validOpcode(uint16_t opcode)
    {
        return opcode < Opcode::MaxOpcode;
    }
};
