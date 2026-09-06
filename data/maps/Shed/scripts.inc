Shed_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, Shed_OnTransition
	map_script MAP_SCRIPT_ON_WARP_INTO_MAP_TABLE, Shed_OnWarpTable
	map_script MAP_SCRIPT_ON_FRAME_TABLE, Shed_OnFrameTable
	.byte 0

Shed_OnTransition:
	goto_if_ge VAR_DADS_FARM_STATE, 6, Shed_EventScript_SetMusicLittleroot
	goto Shed_EventScript_SetMusicHelp
	end

Shed_EventScript_SetMusicHelp:
	setflag FLAG_TEMP_E
	playbgm MUS_HELP, TRUE
	return

Shed_EventScript_SetMusicLittleroot:
	clearflag FLAG_TEMP_E
	playbgm MUS_LITTLEROOT, TRUE
	end

Shed_OnWarpTable:
	map_script_2 VAR_DADS_FARM_STATE, 3, Shed_EventScript_FacePlayerNorth
	.2byte 0

Shed_OnFrameTable:
	map_script_2 VAR_DADS_FARM_STATE, 3, Shed_EventScript_EnterShedStepForward
	.2byte 0

Shed_EventScript_FacePlayerNorth::
	turnobject OBJ_EVENT_ID_PLAYER, DIR_NORTH
	end

Shed_EventScript_EnterShedStepForward::
	lockall
	applymovement OBJ_EVENT_ID_PLAYER, Shed_Movement_StepInside
	waitmovement 0
	setvar VAR_DADS_FARM_STATE, 4
	releaseall
	end

Shed_EventScript_CantLeave::
	msgbox Shed_Text_CantLeave, MSGBOX_DEFAULT
	closemessage
	applymovement OBJ_EVENT_ID_PLAYER, Shed_Movement_PushBack
	waitmovement 0
	releaseall
	end

@ -------------------------------------------------------------
@ STARTER BAG INTERACTION
@ -------------------------------------------------------------
Shed_EventScript_Bag::
	lock
	faceplayer
	setflag FLAG_SYS_POKEMON_GET
	fadescreen FADE_TO_BLACK
	special ChooseStarter
	waitstate
	setvar VAR_DADS_FARM_STATE, 5
  call Shed_EventScript_SetMusicHelp
  setflag FLAG_HIDE_ROUTE_101_OLIVE_STARTERS_BAG
	release
	end

Shed_Movement_StepInside:
	walk_up
	step_end

Shed_Movement_PushBack:
	walk_up
	step_end

Shed_Text_CantLeave:
	.string "Too dangerous out there! I need\n"
	.string "something to protect myself first.$"
