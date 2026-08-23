DadsFarm_House_2F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, DadsFarm_House_2F_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, DadsFarm_House_2F_OnFrame
	.byte 0

DadsFarm_House_2F_OnTransition:
	call_if_eq VAR_DADS_FARM_STATE, 0, DadsFarm_House_2F_EventScript_HideDadOnLoad
	end

DadsFarm_House_2F_EventScript_HideDadOnLoad::
	setflag FLAG_HIDE_LITTLEROOT_TOWN_PLAYERS_BEDROOM_MOM
	return

DadsFarm_House_2F_OnFrame:
	map_script_2 VAR_DADS_FARM_STATE, 0, DadsFarm_House_2F_EventScript_DadSequence
	.2byte 0

DadsFarm_House_2F_EventScript_DadSequence::
	lockall
	
	@ Player tossing and turning in bed
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_House_2F_Movement_PlayerTossing
	waitmovement 0
	
	@ Spawn Dad right when he enters
	clearflag FLAG_HIDE_LITTLEROOT_TOWN_PLAYERS_BEDROOM_MOM
	addobject 1

	@ Move Dad down
	applymovement 1, DadsFarm_House_2F_Movement_DadApproach
	waitmovement 0
	
	@ Dialogue
	msgbox DadsFarm_House_2F_Text_DadSequence, MSGBOX_DEFAULT
	waitmessage
	closemessage

	@ Move Dad back up
	applymovement 1, DadsFarm_House_2F_Movement_DadLeave
	waitmovement 0
	
	removeobject 1
	setflag FLAG_HIDE_LITTLEROOT_TOWN_PLAYERS_BEDROOM_MOM
	setvar VAR_DADS_FARM_STATE, 1  @ Prevents script from triggering again

	@ Set quest to Active
	startquest QUEST_CHORES
	subquestmenu QUEST_MENU_COMPLETE_QUEST, QUEST_CHORES, SUB_QUEST_OUT
	openquestmenu

	releaseall
	end

DadsFarm_House_2F_Movement_PlayerTossing:
	delay_16
	walk_in_place_faster_left
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	walk_in_place_faster_right
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	walk_in_place_faster_down
	delay_16
	delay_16
	delay_16
	walk_in_place_faster_right
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	delay_16
	step_end

DadsFarm_House_2F_Movement_DadApproach:
	delay_16
	walk_down
	walk_down
	walk_down
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

DadsFarm_House_2F_Movement_DadLeave:
	walk_right
	walk_right
	walk_right
	walk_right
	walk_right
	walk_up
	walk_up
	walk_up
	delay_16
	step_end

DadsFarm_House_2F_EventScript_PC::
	lockall
	setvar VAR_0x8004, PC_LOCATION_BRENDANS_HOUSE
	special DoPCTurnOnEffect
	playse SE_PC_ON
	msgbox gText_PlayerHouseBootPC, MSGBOX_DEFAULT
	special BedroomPC
	releaseall
	end

DadsFarm_House_2F_EventScript_TurnOffPlayerPC::
	setvar VAR_0x8004, PC_LOCATION_BRENDANS_HOUSE
	playse SE_PC_OFF
	special DoPCTurnOffEffect
	releaseall
	end

DadsFarm_House_2F_EventScript_PlayerDresser::
	msgbox DadsFarm_House_2F_Text_PlayerDresser, MSGBOX_SIGN
	end

DadsFarm_House_2F_EventScript_MarkosDresser::
	msgbox DadsFarm_House_2F_Text_MarkosDresser, MSGBOX_SIGN
	end

DadsFarm_House_2F_Text_DadSequence:
	.string "{PLAYER}, you're still in bed?\n"
	.string "You have chores to do today!\p"
	.string "Oh, by the way... your Mom is\n"
	.string "in town right now.$"

DadsFarm_House_2F_Text_PlayerDresser:
	.string "It's your dresser!\n"
	.string "Everything is neat and orderly.$"

DadsFarm_House_2F_Text_MarkosDresser:
	.string "It's Markos' dresser!\n"
	.string "It's a bit messy...$"

DadsFarm_House_2F_EventScript_QuestMenu::
	lockall
	msgbox DadsFarm_House_2F_Text_OpenQuestMenu, MSGBOX_YESNO
	goto_if_eq VAR_RESULT, YES, DadsFarm_House_2F_EventScript_DoOpenQuestMenu
	releaseall
	end

DadsFarm_House_2F_EventScript_DoOpenQuestMenu::
	openquestmenu
	releaseall
	end

DadsFarm_House_2F_Text_OpenQuestMenu:
	.string "There's a logbook sitting on top\n"
	.string "of the desk. Open it?$"
