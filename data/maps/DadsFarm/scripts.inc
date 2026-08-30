DadsFarm_MapScripts::
	map_script MAP_SCRIPT_ON_FRAME_TABLE, DadsFarm_OnFrame
	.byte 0

DadsFarm_OnFrame:
  setflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	map_script_2 VAR_DADS_FARM_STATE, 1, DadsFarm_EventScript_OnLoadCutscene
	.2byte 0

DadsFarm_EventScript_Markos::
	lock
	faceplayer
	msgbox DadsFarm_Text_MarkosWeeding, MSGBOX_DEFAULT
	release
	end

DadsFarm_EventScript_OnLoadCutscene::
	lockall
	delay 30
	applymovement 1, DadsFarm_Movement_MarkosLookUp
	waitmovement 0
	delay 30
	applymovement 1, DadsFarm_Movement_MarkosStepUp
	waitmovement 0
	msgbox DadsFarm_Text_MarkosWeeding, MSGBOX_DEFAULT
	closemessage
	applymovement 1, DadsFarm_Movement_MarkosStepDown
	waitmovement 0
	setvar VAR_DADS_FARM_STATE, 2
	releaseall
	end

DadsFarm_Movement_MarkosLookUp:
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_MarkosStepUp:
	walk_up
	step_end

DadsFarm_Movement_MarkosStepDown:
	walk_down
	walk_in_place_faster_down
	step_end

DadsFarm_Text_MarkosWeeding:
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

.set LOCALID_Markos, 1
.set LOCALID_LAMBIGUOUS, 2

DadsFarm_EventScript_TriggerCutscene::
	lockall
	setvar VAR_TEMP_0, 1
	
	@ 1. Pause and optional cry
	delay 16
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	delay 16
	
	@ 2. Walk up 2 tiles (From Y 31 to Y 29)
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_PlayerUp2
	waitmovement 0
	delay 20
	
	@ 3. Turn Right & Question Mark Emote with SE
	playse SE_SELECT
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_QuestionMark
	waitmovement 0
	delay 30
	
	@ 4. Spawn Lambiguous 8 tiles to player's right, make him visible, move 6 left
	getplayerxy VAR_TEMP_1, VAR_TEMP_2
	addvar VAR_TEMP_1, 8
	setobjectxy LOCALID_LAMBIGUOUS, VAR_TEMP_1, 29
	showobjectat LOCALID_LAMBIGUOUS, MAP_DADS_FARM
	clearflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousApproach
	waitmovement 0
	delay 16
	
	@ Play Lambiguous cry & Exclamation mark emote on player with SE
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_ExclamationMark
	waitmovement 0
	delay 24
	
	@ Start the Birch chase music before running
	playbgm MUS_HELP, TRUE

	@ 5. Sprint to door with Lambiguous chasing
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
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase0
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_CommonRunToDoor
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom31::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase1
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee1
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom32::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase2
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee2
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom33::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase3
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee3
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom34::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase4
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee4
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom35::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase5
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee5
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom36::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase6
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee6
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom37::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase7
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee7
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom38::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase8
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee8
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom39::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase9
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee9
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom40::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase10
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee10
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom41::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase11
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee11
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_RunFrom42::
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousFullChase12
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_RunAndFlee12
	goto DadsFarm_EventScript_DoorWarp

DadsFarm_EventScript_DoorWarp::
	waitmovement OBJ_EVENT_ID_PLAYER
	delay 10
	opendoor 36, 8
	waitdooranim
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_WalkIntoDoor
	waitmovement 0
	closedoor 36, 8
	waitdooranim
	setflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	warp MAP_DADS_FARM, 0, 0, 0
	waitstate
	releaseall
  setvar VAR_DADS_FARM_STATE, 2  @ Prevents script from triggering again
	end

@ -------------------------------------------------------------
@ MOVEMENT SCRIPTS
@ -------------------------------------------------------------

DadsFarm_Movement_PlayerUp2:
	walk_up
	walk_up
	step_end

DadsFarm_Movement_QuestionMark:
	walk_in_place_faster_right
	emote_question_mark
	step_end

DadsFarm_Movement_LambiguousApproach:
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	walk_left
	step_end

DadsFarm_Movement_ExclamationMark:
	emote_exclamation_mark
	step_end

DadsFarm_Movement_WalkIntoDoor:
	walk_up
	set_invisible
	step_end

@ Combined Player Movement Paths
DadsFarm_Movement_CommonRunToDoor:
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
	player_run_up
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
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee1:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee2:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee3:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee4:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee5:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee6:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee7:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee8:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee9:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee10:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee11:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_RunAndFlee12:
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
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	player_run_right
	walk_in_place_faster_up
	step_end

@ Synchronized Lambiguous Full Chase Paths
DadsFarm_Movement_LambiguousFullChase0:
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase1:
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase2:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase3:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase4:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase5:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase6:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase7:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase8:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase9:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase10:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase11:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end

DadsFarm_Movement_LambiguousFullChase12:
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_left
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_up
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	walk_fast_right
	step_end
