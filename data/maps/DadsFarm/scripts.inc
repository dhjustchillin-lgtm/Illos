DadsFarm_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, DadsFarm_OnFrame
	.byte 0

DadsFarm_OnFrame:
	map_script_2 VAR_DADS_FARM_STATE, 1, DadsFarm_EventScript_OnLoadCutscene
	.2byte 0

DadsFarm_EventScript_OnLoadCutscene::
	lockall
	delay 30
	applymovement 1, DadsFarm_Movement_BrendanLookUp
	waitmovement 0
	delay 30
	applymovement 1, DadsFarm_Movement_BrendanStepUp
	waitmovement 0
	msgbox DadsFarm_Text_BrendanWeeding, MSGBOX_DEFAULT
	closemessage
	applymovement 1, DadsFarm_Movement_BrendanStepDown
	waitmovement 0
	setvar VAR_DADS_FARM_STATE, 2
	releaseall
	end

DadsFarm_Movement_BrendanLookUp:
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_BrendanStepUp:
	walk_up
	step_end

DadsFarm_Movement_BrendanStepDown:
	walk_down
	walk_in_place_faster_down
	step_end

DadsFarm_Text_BrendanWeeding:
	.string "Oh, hi, {PLAYER}! I already started work\n"
	.string "weeding. Could you go plant some\l"
	.string "berries up north?$"

DadsFarm_EventScript_Sign::
	msgbox DadsFarm_Text_Sign, MSGBOX_SIGN
	end

DadsFarm_Text_Sign:
	.string "Hayes Family Farm\n"
	.string "Owned by:\p"
	.string "Graham\n"
	.string "Father\p"
	.string "Tabitha\n"
	.string "Wife\p"
	.string "Markos\n"
	.string "Youngest Son\p"
	.string "{PLAYER}\n"
	.string "Eldest Son$"
