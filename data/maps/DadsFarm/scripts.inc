.set LOCALID_Markos, 1
.set LOCALID_LAMBIGUOUS, 2
.set LOCALID_DAD, 3
.set LOCALID_Markos_Temp, 4

DadsFarm_MapScripts::
	map_script MAP_SCRIPT_ON_TRANSITION, DadsFarm_OnTransition
	map_script MAP_SCRIPT_ON_FRAME_TABLE, DadsFarm_OnFrameTable
	.byte 0

DadsFarm_OnTransition:
	call_if_eq VAR_DADS_FARM_STATE, 5, DadsFarm_EventScript_PrepareShedAmbush
	end

DadsFarm_EventScript_PrepareShedAmbush::
	@ Reveal Lambiguous BEFORE the map loads so the map loader instantiates the sprite
	clearflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	setobjectxyperm LOCALID_LAMBIGUOUS, 36, 11
	setobjectmovementtype LOCALID_LAMBIGUOUS, MOVEMENT_TYPE_FACE_UP
	return

DadsFarm_OnFrameTable:
	map_script_2 VAR_DADS_FARM_STATE, 1, DadsFarm_EventScript_OnLoadCutscene
	map_script_2 VAR_DADS_FARM_STATE, 5, DadsFarm_EventScript_ShedAmbushCutscene
	.2byte 0

DadsFarm_EventScript_Markos::
	lock
	faceplayer
	msgbox DadsFarm_Text_MarkosWeeding, MSGBOX_DEFAULT
	release
	end

DadsFarm_EventScript_OnLoadCutscene::
	lockall
	setflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	setflag FLAG_HIDE_PLAYERS_HOUSE_DAD @ Ensure Dad is hidden initially
	setflag FLAG_HIDE_ROUTE_104_RIVAL @ Ensure Dad is hidden initially
	delay 30
	applymovement LOCALID_Markos, DadsFarm_Movement_MarkosLookUp
	waitmovement 0
	delay 15
	playse SE_PIN
	applymovement LOCALID_Markos, DadsFarm_Movement_MarkosExclamation
	waitmovement 0
	delay 30
	applymovement LOCALID_Markos, DadsFarm_Movement_MarkosStepUp
	waitmovement 0
	msgbox DadsFarm_Text_MarkosWeeding, MSGBOX_DEFAULT
	closemessage
	applymovement LOCALID_Markos, DadsFarm_Movement_MarkosStepDown
	waitmovement 0
	setvar VAR_DADS_FARM_STATE, 2
	releaseall
	end

@ -------------------------------------------------------------
@ SHED AMBUSH BATTLE CUTSCENE (VAR_DADS_FARM_STATE == 5)
@ -------------------------------------------------------------
DadsFarm_EventScript_ShedAmbushCutscene::
	lockall
	delay 20

	@ Lambiguous cries
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	delay 15

	@ Player turns down and shows Exclamation
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_PlayerExclamationDown
	waitmovement 0
	delay 20

	@ Lambiguous lunges up 1 tile towards player
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousLunge
	waitmovement 0

	@ Initiate wild battle using Route 101 battle parameters
	setwildbattle SPECIES_LAMBIGUOUS, 7, ITEM_NONE
	dowildbattle
	
	@ Post-battle cleanup & Spawning Dad + Markos
	removeobject LOCALID_LAMBIGUOUS
	setflag FLAG_HIDE_ROUTE_101_ZIGZAGOON

	@ 1. Unhide flags first
	clearflag FLAG_HIDE_PLAYERS_HOUSE_DAD
	clearflag FLAG_HIDE_ROUTE_104_RIVAL

	@ 2. Set permanent coordinates before instantiating
	setobjectxyperm LOCALID_DAD, 30, 15
	setobjectxyperm LOCALID_Markos_Temp, 30, 16

	@ 3. Add objects to the active map state
	addobject LOCALID_DAD
	addobject LOCALID_Markos_Temp

	@ 4. Update current coordinates to match
	setobjectxy LOCALID_DAD, 30, 15
	setobjectxy LOCALID_Markos_Temp, 30, 16

	applymovement LOCALID_DAD, DadsFarm_Movement_DadRunToPosition
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_DadRunToPosition
	waitmovement 0

	@ Turn player to face Dad on the left
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_PlayerFaceLeft
	waitmovement 0

	@ Dialogue sequence post-chase
	msgbox DadsFarm_Text_DadPostBattle, MSGBOX_DEFAULT
	closemessage

	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_MarkosLookUp
	waitmovement 0

	msgbox DadsFarm_Text_MarkosAsksForPokemon, MSGBOX_DEFAULT
	closemessage

	msgbox DadsFarm_Text_DadRespondsToMarkos, MSGBOX_DEFAULT
	closemessage

	@ 1. Player moves down, right, and turns left FIRST
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_PlayerStepOut
	waitmovement 0

	@ 2. NOW Markos walks to the shed door
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_MarkosEnterShed
	waitmovement 0

	opendoor 36, 8
	waitdooranim
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_WalkIntoDoor
	waitmovement 0
	closedoor 36, 8
	waitdooranim
	removeobject LOCALID_Markos_Temp

	@ Wait 2 seconds (120 frames), then Markos exits the door
	delay 120

	opendoor 36, 8
	waitdooranim
	addobject LOCALID_Markos_Temp
	setobjectxy LOCALID_Markos_Temp, 36, 8
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_WalkOutOfDoor
	waitmovement 0
	closedoor 36, 8
	waitdooranim

	@ Markos walks down, turns right, and asks to battle
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_MarkosWalkDown
	waitmovement 0
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_FaceRight
	waitmovement 0

	msgbox DadsFarm_Text_MarkosAskBattle, MSGBOX_DEFAULT
	closemessage

	@ Dad shows Exclamation Mark, walks right, and looks down
	playse SE_PIN
	applymovement LOCALID_DAD, DadsFarm_Movement_DadExclamationWalkRightLookDown
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_MarkosLookUp
	waitmovement LOCALID_DAD
	waitmovement LOCALID_Markos_Temp
	delay 20

	@ Dad explains Pokémon are for work, not battling
	msgbox DadsFarm_Text_DadPokemonAreForWork, MSGBOX_DEFAULT
	closemessage

	@ Markos begs "Please!"
	msgbox DadsFarm_Text_MarkosBegsBattle, MSGBOX_DEFAULT
	closemessage

	@ Dad talks FIRST before moving to heal
	msgbox DadsFarm_Text_DadHealsParty, MSGBOX_DEFAULT
	closemessage

	@ Dad JUST moves one right and looks down
	applymovement LOCALID_DAD, DadsFarm_Movement_DadStepRightLookDown
	waitmovement 0

	@ Markos looks right, Player looks up
	applymovement LOCALID_Markos_Temp, DadsFarm_Movement_FaceRight
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_FaceUp
	waitmovement 0

	@ Heal player's party
	special HealPlayerParty

	delay 60 @ Wait a second (60 frames)

	@ Dad walks two left and turns right
	applymovement LOCALID_DAD, DadsFarm_Movement_DadWalkTwoLeftFaceRight
	waitmovement 0

	@ Player turns left
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_FaceLeft
	waitmovement 0

	@ Markos asks "Are you ready?"
	msgbox DadsFarm_Text_MarkosAreYouReady, MSGBOX_DEFAULT
	closemessage

	setvar VAR_DADS_FARM_STATE, 6
	releaseall
	end

@ -------------------------------------------------------------
@ DIALOGUE TEXT STRINGS
@ -------------------------------------------------------------

DadsFarm_Text_DadPostBattle:
	.string "Dad: {PLAYER}! Are you alright?\p"
	.string "We heard all the commotion from over by\n"
	.string "the field, and... wait a minute.\p"
	.string "Why do you have my Pokémon?\p"
	.string "… … … … … … … …\n"
	.string "… … … … … … … …\p"
	.string "Oh. I see.\p"
	.string "Well... hmm. Truth be told, I was hoping\n"
	.string "to train it to help out around the farm,\l"
	.string "but the stubborn thing wouldn't listen\l"
	.string "to a word I said.\p"
	.string "Looks like it took a liking to you, though.\n"
	.string "Alright, son. Take care of it, raise it\l"
	.string "until it fully evolves, and then come\l"
	.string "find me.$"

DadsFarm_Text_MarkosAsksForPokemon:
	.string "Markos: Woah, Dad! That's not fair!\n"
	.string "Can I get a Pokémon of my own, too?$"

DadsFarm_Text_DadRespondsToMarkos:
	.string "Dad: Haha, sure thing, Markos.\n"
	.string "That just saves me twice as much work!$"

DadsFarm_Text_MarkosAskBattle:
	.string "Markos: Hey {PLAYER}! Now that we both\n"
	.string "have Pokémon, let's battle!$"

DadsFarm_Text_DadPokemonAreForWork:
	.string "Dad: Hold on now, Markos. Pokémon on\n"
	.string "this farm are meant for working,\l"
	.string "not for tough battles!$"

DadsFarm_Text_MarkosBegsBattle:
	.string "Markos: Aww, please Dad? Just one quick\n"
	.string "battle! We need to see who's stronger!$"

DadsFarm_Text_DadHealsParty:
	.string "Dad: ...Alright, fine... but make\n"
	.string "sure your Pokémon are fully healed first.$"

DadsFarm_Text_MarkosAreYouReady:
	.string "Markos: Alright, {PLAYER}!\n"
	.string "Are you ready for this?$"

@ -------------------------------------------------------------
@ MOVEMENT SCRIPTS & TEXT
@ -------------------------------------------------------------

DadsFarm_Movement_PlayerExclamationDown:
	walk_in_place_faster_down
	emote_exclamation_mark
	step_end

DadsFarm_Movement_LambiguousLunge:
	walk_fast_up
	step_end

DadsFarm_Movement_PlayerFaceLeft:
	walk_in_place_faster_left
	step_end

DadsFarm_Movement_PlayerStepOut:
	walk_down
	walk_right
	walk_in_place_faster_left
	step_end

DadsFarm_Movement_MarkosEnterShed:
	walk_right
	walk_up
	step_end

DadsFarm_Movement_WalkOutOfDoor:
	walk_down
	set_visible
	step_end

DadsFarm_Movement_MarkosWalkDown:
	walk_down
	step_end

DadsFarm_Movement_FaceRight:
	walk_in_place_faster_right
	step_end

DadsFarm_Movement_FaceLeft:
	walk_in_place_faster_left
	step_end

DadsFarm_Movement_FaceUp:
	walk_in_place_faster_up
	step_end

DadsFarm_Movement_DadExclamationWalkRightLookDown:
	emote_exclamation_mark
	walk_right
	walk_in_place_faster_down
	step_end

DadsFarm_Movement_DadStepRightLookDown:
	walk_right
	walk_in_place_faster_down
	step_end

DadsFarm_Movement_DadWalkTwoLeftFaceRight:
	walk_left
	walk_left
	walk_in_place_faster_right
	step_end

DadsFarm_Movement_DadRunToPosition:
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

DadsFarm_Movement_MarkosExclamation:
	emote_exclamation_mark
	step_end

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
	.string "berries out east?$"

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
@ INITIAL CHASE TRIGGER
@ -------------------------------------------------------------
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
	
	@ 3. Spawn & make Lambiguous visible BEFORE emote
	getplayerxy VAR_TEMP_1, VAR_TEMP_2
	addvar VAR_TEMP_1, 8
	clearflag FLAG_HIDE_ROUTE_101_ZIGZAGOON
	addobject LOCALID_LAMBIGUOUS
	setobjectxy LOCALID_LAMBIGUOUS, VAR_TEMP_1, 29
	showobjectat LOCALID_LAMBIGUOUS, MAP_DADS_FARM
	
	@ 4. Turn Right & Question Mark Emote on player
	playse SE_SELECT
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_QuestionMark
	waitmovement 0
	delay 20
	
	@ 5. Move Lambiguous left toward player
	applymovement LOCALID_LAMBIGUOUS, DadsFarm_Movement_LambiguousApproach
	waitmovement 0
	delay 16
	
	@ 6. Cry & Exclamation Mark on player
	playmoncry SPECIES_LAMBIGUOUS, CRY_MODE_NORMAL
	waitmoncry
	playse SE_PIN
	applymovement OBJ_EVENT_ID_PLAYER, DadsFarm_Movement_ExclamationMark
	waitmovement 0
	delay 24
	
	@ 7. Start Chase BGM & Run
	playbgm MUS_HELP, TRUE

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
	setvar VAR_DADS_FARM_STATE, 3
	warp MAP_SHED, 0, 0, 0
	waitstate
	releaseall
	end

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
	step_end

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
