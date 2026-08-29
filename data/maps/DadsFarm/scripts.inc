@ Define object event local IDs based on index order (1-indexed)
.set LOCALID_BRENDAN, 1
.set LOCALID_LAMBIGUOUS, 2

DadsFarm_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, DadsFarm_OnFrame
	.byte 0

DadsFarm_OnFrame:
	map_script_2 VAR_DADS_FARM_STATE, 1, DadsFarm_EventScript_OnLoadCutscene
	.2byte 0

DadsFarm_EventScript_OnLoadCutscene::
	lockall
	delay 30
	applymovement LOCALID_BRENDAN, DadsFarm_Movement_BrendanLookUp
	waitmovement 0
	delay 30
	applymovement LOCALID_BRENDAN, DadsFarm_Movement_BrendanStepUp
	waitmovement 0
	msgbox DadsFarm_Text_BrendanWeeding, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_BRENDAN, DadsFarm_Movement_BrendanStepDown
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

@ -------------------------------------------------------------
@ CUTSCENE TRIGGER SCRIPT
@ -------------------------------------------------------------

DadsFarm_EventScript_TriggerCutscene::
	lockall
	setvar VAR_TEMP_0, 1
	
	@ 1. Pause and play optional cry
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	
	@ 2. Walk up to Y 39 (From Y 32, this is 7 steps up)
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_PlayerToY39
	waitmovement 0
	
	@ 3. Turn Right & Question Mark Emote
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_QuestionMark
	waitmovement 0
	
	@ 4. Lambiguous moves 2 tiles to the right of Player's current X position
	getplayerxy VAR_TEMP_1, VAR_TEMP_2
	addvar VAR_TEMP_1, 2
	setobjectxy LOCALID_LAMBIGUOUS, VAR_TEMP_1, 39
	turnobject LOCALID_LAMBIGUOUS, DIR_WEST
	
	@ Play Lambiguous cry & Exclamation mark emote on player
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_ExclamationMark
	waitmovement 0
	
	@ 5. Sprint to (36, 8) avoiding (32, 11) -> (40, 20) by looping around X <= 31
	getplayerxy VAR_TEMP_1, VAR_TEMP_2
	goto_if_eq VAR_TEMP_1, 30, DadsFarm_EventScript_RunFrom30
	goto_if_eq VAR_TEMP_1, 31, DadsFarm_EventScript_RunFrom31
	goto_if_eq VAR_TEMP_1, 32, DadsFarm_EventScript_RunFrom32
	goto_if_eq VAR_TEMP_1, 33, DadsFarm_EventScript_RunFrom33
	goto_if_eq VAR_TEMP_1, 34, DadsFarm_EventScript_RunFrom34
	goto_if_eq VAR_TEMP_1, 35, DadsFarm_EventScript_RunFrom35
	goto_if_eq VAR_TEMP_1, 36, DadsFarm_EventScript_RunFrom36
	goto_if_eq VAR_TEMP_1, 37, DadsFarm_EventScript_RunFrom37
	goto_if_eq VAR_TEMP_1, 38, DadsFarm_EventScript_RunFrom38
	goto_if_eq VAR_TEMP_1, 39, DadsFarm_EventScript_RunFrom39
	goto_if_eq VAR_TEMP_1, 40, DadsFarm_EventScript_RunFrom40
	goto_if_eq VAR_TEMP_1, 41, DadsFarm_EventScript_RunFrom41
	goto DadsFarm_EventScript_RunFrom42

DadsFarm_EventScript_RunFrom30::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From30
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom31::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From31
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom32::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From32
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom33::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From33
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom34::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From34
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom35::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From35
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom36::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From36
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom37::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From37
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom38::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From38
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom39::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From39
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom40::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From40
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom41::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From41
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom42::
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunToDoor_From42
	waitmovement 0
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_DoorWarp::
	opendoor 36, 8
	waitdooranim
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_WalkIntoDoor
	waitmovement 0
	closedoor 36, 8
	waitdooranim
	warp MAP_DADS_FARM, 0, 0, 0
	waitstate
	releaseall
	end

@ -------------------------------------------------------------
@ MOVEMENT SCRIPTS
@ -------------------------------------------------------------

DadsFarm_Movement_PlayerToY39:
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	walk_up
	step_end

DadsFarm_Movement_QuestionMark:
	walk_in_place_faster_right
	emote_question_mark
	step_end

DadsFarm_Movement_ExclamationMark:
	emote_exclamation_mark
	step_end

DadsFarm_Movement_WalkIntoDoor:
	walk_up
	set_invisible
	step_end

@ Pathing Routines: Safely route around bounding box (X: 32..40, Y: 11..20)
DadsFarm_Movement_RunToDoor_From30:
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From31:
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From32:
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From33:
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From34:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From35:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From36:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From37:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From38:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From39:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From40:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From41:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end

DadsFarm_Movement_RunToDoor_From42:
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_left
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_up
	step_end
