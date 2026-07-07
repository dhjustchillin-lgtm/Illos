DadsFarm_House_2F_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, DadsFarm_House_2F_OnTransition
	.byte 0

DadsFarm_House_2F_OnTransition:
	@ Blocks the player from leaving until they set the wall clock at the start of the game
	call_if_eq VAR_LITTLEROOT_INTRO_STATE, 4, PlayersHouse_2F_EventScript_BlockStairsUntilClockIsSet
	end

@ --- Object & Tile Interactions ---

DadsFarm_House_2F_EventScript_PC::
	lockall
	setvar VAR_0x8004, PC_LOCATION_BRENDANS_HOUSE
	special DoPCTurnOnEffect
	playse SE_PC_ON
	msgbox gText_PlayerHouseBootPC, MSGBOX_DEFAULT
	special BedroomPC
	releaseall
	end

DadsFarm_House_2F_EventScript_Notebook::
	msgbox DadsFarm_House_2F_Text_Notebook, MSGBOX_SIGN
	end

DadsFarm_House_2F_EventScript_YourDresser::
	msgbox DadsFarm_House_2F_Text_YourDresser, MSGBOX_SIGN
	end

DadsFarm_House_2F_EventScript_MarkosDresser::
	msgbox DadsFarm_House_2F_Text_MarkosDresser, MSGBOX_SIGN
	end

DadsFarm_House_2F_EventScript_WallClock::
	lockall
	msgbox DadsFarm_House_2F_Text_ClockIsStopped, MSGBOX_DEFAULT
	releaseall
	end

@ --- Text Strings ---

DadsFarm_House_2F_Text_YourDresser:
	.string "It's your dresser!\n"
	.string "It's full of clean clothes.$"

DadsFarm_House_2F_Text_MarkosDresser:
	.string "It's Markos' dresser!\n"
	.string "It's a bit disorganized...$"

DadsFarm_House_2F_Text_ClockIsStopped:
	.string "The clock is stopped…\p"
	.string "Better set it and start it!$"

DadsFarm_House_2F_Text_Notebook:
	.string "{PLAYER} flipped open the notebook.\p"
	.string "ADVENTURE RULE NO. 1\n"
	.string "Open the MENU with START.\p"
	.string "ADVENTURE RULE NO. 2\n"
	.string "Record your progress with SAVE.\p"
	.string "The remaining pages are blank…$"
