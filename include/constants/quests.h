#ifndef GUARD_CONSTANTS_QUESTS_H
#define GUARD_CONSTANTS_QUESTS_H

// questmenu scripting command params
#define QUEST_MENU_OPEN                 0
#define QUEST_MENU_UNLOCK_QUEST         1
#define QUEST_MENU_SET_ACTIVE           2
#define QUEST_MENU_SET_REWARD           3
#define QUEST_MENU_COMPLETE_QUEST       4
#define QUEST_MENU_CHECK_UNLOCKED       5
#define QUEST_MENU_CHECK_INACTIVE       6
#define QUEST_MENU_CHECK_ACTIVE         7
#define QUEST_MENU_CHECK_REWARD         8
#define QUEST_MENU_CHECK_COMPLETE       9
#define QUEST_MENU_BUFFER_QUEST_NAME    10

// quest number defines
#define QUEST_CHORES                    0
#define QUEST_COUNT                     (QUEST_CHORES + 1)

#define SUB_QUEST_OUT                  0

#define QUEST_CHORES_SUB_COUNT         1
#define SUB_QUEST_COUNT                QUEST_CHORES_SUB_COUNT

#define QUEST_ARRAY_COUNT              (SUB_QUEST_COUNT > QUEST_COUNT ? SUB_QUEST_COUNT : QUEST_COUNT)

#endif // GUARD_CONSTANTS_QUESTS_H
