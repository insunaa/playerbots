#include "botpch.h"
#include "../../playerbot.h"
#include "DropQuestAction.h"


using namespace ai;

bool DropQuestAction::Execute(Event event)
{
    string link = event.getParam();
    if (!GetMaster())
        return false;

    PlayerbotChatHandler handler(GetMaster());
    uint32 entry = handler.extractQuestId(link);

    // remove all quest entries for 'entry' from quest log
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 logQuest = bot->GetQuestSlotQuestId(slot);
        Quest const* quest = sObjectMgr.GetQuestTemplate(logQuest);
        if (!quest)
            continue;

        if (logQuest == entry || link.find(quest->GetTitle()) != string::npos)
        {
            bot->SetQuestSlot(slot, 0);

            // we ignore unequippable quest items in this case, its' still be equipped
            bot->TakeQuestSourceItem(logQuest, false);
            entry = logQuest;
            break;
        }
    }

    if (!entry)
        return false;

    bot->SetQuestStatus(entry, QUEST_STATUS_NONE);
    bot->getQuestStatusMap()[entry].m_rewarded = false;

    ai->TellMaster("Quest removed");
    return true;
}

bool CleanQuestLogAction::Execute(Event event)
{
    string link = event.getParam();
    if (ai->HasActivePlayerMaster())
        return false;

    uint8 totalQuests = 0;

    DropQuestType(totalQuests); //Count the total quests
      
    if (MAX_QUEST_LOG_SIZE - totalQuests > 6)
        return true;

    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 6); //Drop gray quests.
    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 6, false, true); //Drop gray quests with progress.
    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 6, false, true, true); //Drop gray completed quests.

    if (MAX_QUEST_LOG_SIZE - totalQuests > 4)
        return true;

    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 4, true); //Drop quests without progress.

    if (MAX_QUEST_LOG_SIZE - totalQuests > 2)
        return true;

    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 2, true, true); //Drop quests with progress.

    if (MAX_QUEST_LOG_SIZE - totalQuests > 0)
        return true;

    DropQuestType(totalQuests, MAX_QUEST_LOG_SIZE - 1, true, true, true); //Drop completed quests.

    if (MAX_QUEST_LOG_SIZE - totalQuests > 0)
        return true;

    return false;
}

void CleanQuestLogAction::DropQuestType(uint8 &numQuest, uint8 wantNum, bool isGreen, bool hasProgress, bool isComplete)
{
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);

        if (!questId)
            continue;

        Quest const* quest = sObjectMgr.GetQuestTemplate(questId);
        if (!quest)
            continue;

        if (wantNum == 100)
            numQuest++;

        int32 lowLevelDiff = sWorld.getConfig(CONFIG_INT32_QUEST_LOW_LEVEL_HIDE_DIFF);
        if ((lowLevelDiff < 0 || bot->getLevel() <= bot->GetQuestLevelForPlayer(quest) + uint32(lowLevelDiff)) && !isGreen)
            continue;

        if (HasProgress(questId) && !hasProgress)
            continue;

        if (bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE && !isComplete)
            continue;

        if (numQuest <= wantNum)
            continue;

        //Drop quest.
        bot->SetQuestSlot(slot, 0);

        //We ignore unequippable quest items in this case, its' still be equipped
        bot->TakeQuestSourceItem(questId, false);

        bot->SetQuestStatus(questId, QUEST_STATUS_NONE);
        bot->getQuestStatusMap()[questId].m_rewarded = false;

        numQuest--;

        ai->TellMaster("Quest removed" + chat->formatQuest(quest));
    }
}

bool CleanQuestLogAction::HasProgress(uint32 questId)
{
    if (bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
        return true;

    Quest const* questTemplate = sObjectMgr.GetQuestTemplate(questId);
    QuestStatusData questStatus = bot->getQuestStatusMap()[questId];

    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
    {
        if (!questTemplate->ObjectiveText[i].empty())
            return true;

        if (questTemplate->ReqItemId[i])
        {
            int required = questTemplate->ReqItemCount[i];
            int available = questStatus.m_itemcount[i];
            if (available > 0 && required > 0)
                return true;
        }

        if (questTemplate->ReqCreatureOrGOId[i])
        {
            int required = questTemplate->ReqCreatureOrGOCount[i];
            int available = questStatus.m_creatureOrGOcount[i];

            if (available > 0 && required > 0)
                return true;
        }
    }

    return false;
}