// QuestManager - Quest templates and progression logic
// Phase 7, Task 7.6+: Quest System

#include "stdafx.h"
#include "Systems/QuestManager.h"
#include "Systems/PlayerQuestLog.h"
#include "Systems/Inventory.h"
#include "Systems/ExperienceSystem.h"
#include "World/Player.h"
#include "World/Npc.h"
#include "World/WorldManager.h"
#include "Database/GameData.h"
#include "Core/Logger.h"
#include "GamePacketServer.h"
#include "StlBuffer.h"
#include "PlayerDefines.h"

namespace Quest
{

namespace
{
constexpr int MAX_ACTIVE_QUESTS = 25;

void sendWorldError(Player* player, PlayerDefines::WorldError error)
{
    if (!player)
        return;

    GP_Server_WorldError packet;
    packet.m_code = static_cast<uint8_t>(error);

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    player->sendPacket(buf);
}

bool isRepeatable(const QuestTemplate& quest)
{
    return (quest.flags & static_cast<int32_t>(QuestDefines::Flags::QuestFlagRepeatable)) != 0;
}
} // namespace

QuestManager& QuestManager::instance()
{
    static QuestManager instance;
    return instance;
}

std::vector<QuestObjective> QuestManager::buildObjectives(const QuestTemplate& quest) const
{
    std::vector<QuestObjective> objectives;
    objectives.reserve(4);

    for (int i = 0; i < 4; ++i)
    {
        int32_t count = quest.reqCount[i] > 0 ? quest.reqCount[i] : 1;
        if (quest.reqItem[i] > 0)
        {
            objectives.push_back({QuestDefines::TallyItem, quest.reqItem[i], count, i});
        }
        else if (quest.reqNpc[i] > 0)
        {
            objectives.push_back({QuestDefines::TallyNpc, quest.reqNpc[i], count, i});
        }
        else if (quest.reqGo[i] > 0)
        {
            objectives.push_back({QuestDefines::TallyGameObj, quest.reqGo[i], count, i});
        }
        else if (quest.reqSpell[i] > 0)
        {
            objectives.push_back({QuestDefines::TallySpell, quest.reqSpell[i], count, i});
        }
    }

    return objectives;
}

bool QuestManager::areObjectivesComplete(const QuestTemplate& quest,
                                         const std::array<int32_t, 4>& progress) const
{
    auto objectives = buildObjectives(quest);
    if (objectives.empty())
        return true;

    for (const auto& obj : objectives)
    {
        if (obj.slotIndex < 0 || obj.slotIndex >= static_cast<int32_t>(progress.size()))
            continue;
        if (progress[obj.slotIndex] < obj.requiredCount)
            return false;
    }

    return true;
}

bool QuestManager::isQuestAvailable(Player* player, int32_t questId) const
{
    if (!player)
        return false;

    const QuestTemplate* quest = sGameData.getQuest(questId);
    if (!quest)
        return false;

    if (player->getLevel() < quest->minLevel)
        return false;

    const auto& log = player->getQuestLog();
    const QuestState* state = log.getQuest(questId);
    if (state)
        return false;

    for (int i = 0; i < 3; ++i)
    {
        int32_t prev = quest->prevQuest[i];
        if (prev <= 0)
            continue;
        const QuestState* prevState = log.getQuest(prev);
        if (!prevState || prevState->status != QuestDefines::Status::Rewarded)
            return false;
    }

    return true;
}

void QuestManager::getQuestsForNpc(Player* player, Npc* npc,
                                   std::vector<int32_t>& outOffers,
                                   std::vector<int32_t>& outCompletes) const
{
    outOffers.clear();
    outCompletes.clear();

    if (!player || !npc)
        return;

    int32_t npcEntry = npc->getEntry();
    const auto& quests = sGameData.getAllQuests();
    const auto& log = player->getQuestLog();

    for (const auto& [questId, quest] : quests)
    {
        if (quest.startNpcEntry == npcEntry && isQuestAvailable(player, questId))
        {
            outOffers.push_back(questId);
        }

        if (quest.finishNpcEntry == npcEntry)
        {
            const QuestState* state = log.getQuest(questId);
            if (state && state->status == QuestDefines::Status::Complete)
            {
                outCompletes.push_back(questId);
            }
        }
    }
}

ObjDefines::GossipStatus QuestManager::getGossipStatus(Player* player, Npc* npc) const
{
    if (!player || !npc || !npc->isQuestGiver())
        return ObjDefines::GossipStatus::None;

    std::vector<int32_t> offers;
    std::vector<int32_t> completes;
    getQuestsForNpc(player, npc, offers, completes);

    if (!completes.empty())
        return ObjDefines::GossipStatus::QuestComplete;
    if (!offers.empty())
        return ObjDefines::GossipStatus::QuestAvailable;

    return ObjDefines::GossipStatus::GossipAvailable;
}

void QuestManager::sendQuestTally(Player* player, int32_t questId, QuestDefines::TallyType type,
                                  int32_t targetId, int32_t progress) const
{
    if (!player)
        return;

    GP_Server_QuestTally packet;
    packet.m_questId = questId;
    packet.m_type = static_cast<uint8_t>(type);
    packet.m_entry = targetId;
    packet.m_tally = progress;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    player->sendPacket(buf);
}

void QuestManager::updateQuestCompletion(Player* player, const QuestTemplate& quest)
{
    if (!player)
        return;

    auto* state = player->getQuestLog().getQuest(quest.entry);
    if (!state)
        return;

    bool complete = areObjectivesComplete(quest, state->progress);

    if (complete && state->status != QuestDefines::Status::Complete)
    {
        player->getQuestLog().setStatus(quest.entry, QuestDefines::Status::Complete);

        GP_Server_QuestComplete packet;
        packet.m_questId = quest.entry;
        packet.m_done = true;

        StlBuffer buf;
        uint16_t opcode = packet.getOpcode();
        buf << opcode;
        packet.pack(buf);
        player->sendPacket(buf);

        refreshNpcGossipStatuses(player);
    }
    else if (!complete && state->status == QuestDefines::Status::Complete)
    {
        player->getQuestLog().setStatus(quest.entry, QuestDefines::Status::InProgress);

        GP_Server_QuestComplete packet;
        packet.m_questId = quest.entry;
        packet.m_done = false;

        StlBuffer buf;
        uint16_t opcode = packet.getOpcode();
        buf << opcode;
        packet.pack(buf);
        player->sendPacket(buf);

        refreshNpcGossipStatuses(player);
    }
}

void QuestManager::refreshNpcGossipStatuses(Player* player) const
{
    if (!player)
        return;

    // NOTE: GP_Server_ObjectVariable is NOT supported by the client binary.
    // Instead, we resend the full NPC packet which includes DynGossipStatus.
    // WorldManager::sendNpcTo() already includes the gossip status from getGossipStatus().
    auto npcs = sWorldManager.getNpcsOnMap(player->getMapId());
    for (Npc* npc : npcs)
    {
        if (!npc || !npc->isQuestGiver())
            continue;

        // Resend full NPC packet to this player - includes updated gossip status
        sWorldManager.sendNpcTo(player, npc);
    }
}

void QuestManager::onNpcKilled(Player* player, int32_t npcEntry)
{
    if (!player)
        return;

    auto& log = player->getQuestLog();
    for (auto& [questId, state] : log.getQuests())
    {
        if (state.status != QuestDefines::Status::InProgress)
            continue;

        const QuestTemplate* quest = sGameData.getQuest(questId);
        if (!quest)
            continue;

        auto objectives = buildObjectives(*quest);
        bool updated = false;

        for (const auto& obj : objectives)
        {
            if (obj.type != QuestDefines::TallyNpc || obj.targetId != npcEntry)
                continue;

            int32_t current = state.progress[obj.slotIndex];
            int32_t next = std::min(obj.requiredCount, current + 1);
            if (next != current)
            {
                state.progress[obj.slotIndex] = next;
                log.markDirty();
                sendQuestTally(player, questId, obj.type, obj.targetId, next);
                updated = true;
            }
        }

        if (updated)
            updateQuestCompletion(player, *quest);
    }
}

void QuestManager::onInventoryChanged(Player* player)
{
    if (!player)
        return;

    auto& log = player->getQuestLog();

    for (auto& [questId, state] : log.getQuests())
    {
        if (state.status != QuestDefines::Status::InProgress &&
            state.status != QuestDefines::Status::Complete)
            continue;

        const QuestTemplate* quest = sGameData.getQuest(questId);
        if (!quest)
            continue;

        auto objectives = buildObjectives(*quest);
        bool updated = false;

        for (const auto& obj : objectives)
        {
            if (obj.type != QuestDefines::TallyItem)
                continue;

            int32_t count = player->getInventory().countItem(obj.targetId);
            int32_t next = std::min(obj.requiredCount, count);
            int32_t current = state.progress[obj.slotIndex];

            if (next != current)
            {
                state.progress[obj.slotIndex] = next;
                log.markDirty();
                sendQuestTally(player, questId, obj.type, obj.targetId, next);
                updated = true;
            }
        }

        if (updated)
            updateQuestCompletion(player, *quest);
    }
}

void QuestManager::onSpellCast(Player* player, int32_t spellId)
{
    if (!player)
        return;

    auto& log = player->getQuestLog();
    for (auto& [questId, state] : log.getQuests())
    {
        if (state.status != QuestDefines::Status::InProgress)
            continue;

        const QuestTemplate* quest = sGameData.getQuest(questId);
        if (!quest)
            continue;

        auto objectives = buildObjectives(*quest);
        bool updated = false;

        for (const auto& obj : objectives)
        {
            if (obj.type != QuestDefines::TallySpell || obj.targetId != spellId)
                continue;

            int32_t current = state.progress[obj.slotIndex];
            int32_t next = std::min(obj.requiredCount, current + 1);
            if (next != current)
            {
                state.progress[obj.slotIndex] = next;
                log.markDirty();
                sendQuestTally(player, questId, obj.type, obj.targetId, next);
                updated = true;
            }
        }

        if (updated)
            updateQuestCompletion(player, *quest);
    }
}

bool QuestManager::acceptQuest(Player* player, int32_t questId)
{
    if (!player)
        return false;

    const QuestTemplate* quest = sGameData.getQuest(questId);
    if (!quest)
        return false;

    // Active quest cap
    int activeCount = 0;
    for (const auto& [id, state] : player->getQuestLog().getQuests())
    {
        if (state.status == QuestDefines::Status::InProgress ||
            state.status == QuestDefines::Status::Complete)
            ++activeCount;
    }

    if (activeCount >= MAX_ACTIVE_QUESTS)
    {
        sendWorldError(player, PlayerDefines::WorldError::MaximumQuestsTracked);
        return false;
    }

    if (!isQuestAvailable(player, questId))
    {
        sendWorldError(player, PlayerDefines::WorldError::QuestNotAvailable);
        return false;
    }

    if (!player->getQuestLog().addQuest(questId))
        return false;

    // Provide starter item if required
    if (quest->providedItem > 0)
    {
        int slot = player->getInventory().addItem(quest->providedItem, 1);
        if (slot < 0)
        {
            player->getQuestLog().removeQuest(questId);
            sendWorldError(player, PlayerDefines::WorldError::NoInventorySpace);
            return false;
        }
    }

    GP_Server_AcceptedQuest packet;
    packet.m_questId = questId;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    player->sendPacket(buf);

    updateQuestCompletion(player, *quest);
    refreshNpcGossipStatuses(player);
    return true;
}

bool QuestManager::removeRequiredItems(Player* player, const QuestTemplate& quest) const
{
    if (!player)
        return false;

    // Verify items first
    for (int i = 0; i < 4; ++i)
    {
        if (quest.reqItem[i] <= 0)
            continue;

        int32_t required = quest.reqCount[i] > 0 ? quest.reqCount[i] : 1;
        if (!player->getInventory().hasItem(quest.reqItem[i], required))
            return false;
    }

    // Remove items (suppress quest updates during removal)
    auto& inventory = player->getInventory();
    inventory.setNotificationsEnabled(false);

    for (int i = 0; i < 4; ++i)
    {
        if (quest.reqItem[i] <= 0)
            continue;

        int32_t remaining = quest.reqCount[i] > 0 ? quest.reqCount[i] : 1;
        for (int slot = 0; slot < Inventory::MAX_SLOTS && remaining > 0; ++slot)
        {
            const auto* item = inventory.getItem(slot);
            if (!item || item->itemId != quest.reqItem[i])
                continue;

            int32_t removeCount = std::min(remaining, item->stackCount);
            inventory.removeItem(slot, removeCount);
            remaining -= removeCount;
        }
    }

    inventory.setNotificationsEnabled(true);
    player->onInventoryChanged();

    return true;
}

bool QuestManager::canReceiveRewards(Player* player, const QuestTemplate& quest,
                                     int32_t rewardChoice) const
{
    if (!player)
        return false;

    int32_t choiceCount = 0;
    for (int i = 0; i < 4; ++i)
    {
        if (quest.rewChoiceItem[i] > 0)
            ++choiceCount;
    }

    if (choiceCount > 0 && (rewardChoice < 0 || rewardChoice >= choiceCount))
    {
        sendWorldError(player, PlayerDefines::WorldError::RewardNotChosen);
        return false;
    }

    int requiredSlots = 0;
    auto addRewardSlots = [&requiredSlots](int32_t itemId, int32_t count)
    {
        if (itemId <= 0 || count <= 0)
            return;
        const ItemTemplate* tmpl = sGameData.getItem(itemId);
        int32_t maxStack = tmpl && tmpl->stackCount > 0 ? tmpl->stackCount : 1;
        requiredSlots += (count + maxStack - 1) / maxStack;
    };

    for (int i = 0; i < 4; ++i)
    {
        addRewardSlots(quest.rewItem[i], quest.rewItemCount[i] > 0 ? quest.rewItemCount[i] : 1);
    }

    if (choiceCount > 0)
    {
        int choiceIndex = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (quest.rewChoiceItem[i] <= 0)
                continue;
            if (choiceIndex == rewardChoice)
            {
                addRewardSlots(quest.rewChoiceItem[i],
                               quest.rewChoiceCount[i] > 0 ? quest.rewChoiceCount[i] : 1);
                break;
            }
            ++choiceIndex;
        }
    }

    if (player->getInventory().countEmptySlots() < requiredSlots)
    {
        sendWorldError(player, PlayerDefines::WorldError::NoInventorySpace);
        return false;
    }

    return true;
}

void QuestManager::grantRewards(Player* player, const QuestTemplate& quest, int32_t rewardChoice) const
{
    if (!player)
        return;

    if (quest.rewMoney > 0)
        player->addGold(quest.rewMoney);

    if (quest.rewXp > 0)
        sExperienceSystem.giveExperience(player, quest.rewXp, "quest");

    for (int i = 0; i < 4; ++i)
    {
        if (quest.rewItem[i] <= 0)
            continue;

        int32_t count = quest.rewItemCount[i] > 0 ? quest.rewItemCount[i] : 1;
        player->getInventory().addItem(quest.rewItem[i], count);
    }

    if (rewardChoice >= 0)
    {
        int choiceIndex = 0;
        for (int i = 0; i < 4; ++i)
        {
            if (quest.rewChoiceItem[i] <= 0)
                continue;
            if (choiceIndex == rewardChoice)
            {
                int32_t count = quest.rewChoiceCount[i] > 0 ? quest.rewChoiceCount[i] : 1;
                player->getInventory().addItem(quest.rewChoiceItem[i], count);
                break;
            }
            ++choiceIndex;
        }
    }
}

bool QuestManager::completeQuest(Player* player, int32_t questId, int32_t rewardChoice)
{
    if (!player)
        return false;

    auto* state = player->getQuestLog().getQuest(questId);
    if (!state || state->status != QuestDefines::Status::Complete)
    {
        sendWorldError(player, PlayerDefines::WorldError::QuestNotDone);
        return false;
    }

    const QuestTemplate* quest = sGameData.getQuest(questId);
    if (!quest)
        return false;

    if (!canReceiveRewards(player, *quest, rewardChoice))
        return false;

    if (!removeRequiredItems(player, *quest))
    {
        sendWorldError(player, PlayerDefines::WorldError::MissingItem);
        return false;
    }

    grantRewards(player, *quest, rewardChoice);

    GP_Server_RewardedQuest packet;
    packet.m_questId = questId;
    packet.m_rewardChoice = rewardChoice;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    player->sendPacket(buf);

    if (isRepeatable(*quest))
    {
        player->getQuestLog().removeQuest(questId);
    }
    else
    {
        player->getQuestLog().setStatus(questId, QuestDefines::Status::Rewarded);
    }

    refreshNpcGossipStatuses(player);
    return true;
}

bool QuestManager::abandonQuest(Player* player, int32_t questId)
{
    if (!player)
        return false;

    if (!player->getQuestLog().hasQuest(questId))
        return false;

    player->getQuestLog().removeQuest(questId);

    GP_Server_AbandonQuest packet;
    packet.m_questId = questId;

    StlBuffer buf;
    uint16_t opcode = packet.getOpcode();
    buf << opcode;
    packet.pack(buf);
    player->sendPacket(buf);

    refreshNpcGossipStatuses(player);
    return true;
}

} // namespace Quest
