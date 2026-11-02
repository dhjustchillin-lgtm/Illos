#include "global.h"
#include "main.h"
#include "palette.h"
#include "scanline_effect.h"
#include "task.h"
#include "title_screen.h"
#include "libgcnmultiboot.h"
#include "malloc.h"
#include "gpu_regs.h"
#include "link.h"
#include "multiboot_pokemon_colosseum.h"
#include "load_save.h"
#include "save.h"
#include "new_game.h"
#include "m4a.h"
#include "random.h"
#include "decompress.h"
#include "constants/songs.h"
#include "intro_credits_graphics.h"
#include "trig.h"
#include "intro.h"
#include "graphics.h"
#include "sound.h"
#include "util.h"
#include "title_screen.h"
#include "expansion_intro.h"
#include "battle_anim.h"
#include "intro_frlg.h"
#include "constants/rgb.h"
#include "constants/battle_anim.h"
#include "pokemon.h"
#include "dynamic_palettes.h"
#include "trainer_pokemon_sprites.h"

/*
The intro is grouped into the following scenes
Scene 0. Copyright screen
Scene 1. GF Logo, pan up over plants, Flygon silhouette goes by
Scene 2. Player biking on path, joined by Pokémon
Scene 3. A fight between Groudon/Kyogre ends with Rayquaza

After this it progresses to the title screen

*/

// Scene 1 main tasks
static void Task_Scene1_FadeIn(u8);
static void Task_Scene1_WaterDrops(u8);
static void Task_Scene1_PanUp(u8);
static void Task_Scene1_End(u8);
static void Task_Test1_Water(u8);
static void IntroWaterHBlankCallback(void);
static u16 GetTest1Horizon(void);
static void Task_Test1_Transition(u8);

void Task_Scene1_Load(u8 taskId);
static void Task_Scene1_LoadPrepare(u8 taskId);
static void Task_Scene1_LoadGraphics(u8 taskId);
static void Task_Test1_Show(u8 taskId);

static void Task_Test2_Load(u8 taskId);
static void Task_Test2_LoadPrepare(u8 taskId);
static void Task_Test2_LoadGraphics(u8 taskId);
static void Task_Test2_Show(u8 taskId);

static void Task_Test2_ShowFrame(u8 taskId);

static void Task_Test2_Opening(u8 taskId);
static void Task_Test3_LoadMountains(u8 taskId);
static void Task_Test3_HoldMountain(u8 taskId);
static void Test2ApplySharedDisplacement(s16 displacement);

static void PrepareIntroSceneTransition(void);

static void Task_Test2_MonSequence(u8 taskId);
static void Task_Test2_MonRelease(u8 taskId);
static void Task_Test2_MonHold(u8 taskId);
static void Task_Test2_MonReturn(u8 taskId);
static void Task_Test2_LoadMonBackground(u8 taskId);
static void Task_Test2_RestoreRunningScene(u8 taskId);
static void Task_Test2_CreateRunningSprites(u8 taskId);
static void Task_Test2_HideRunningSprites(u8 taskId);
static void Task_Test2_ShowRunningSprites(u8 taskId);
static void Task_Test2_PlayMonCry(u8 taskId);
static void Test2SetCinematicBars(u8 barHeight);

enum
{
TEST1_MOUNTAIN_MOVING,
TEST1_MOUNTAIN_HOLDING,
TEST1_MOUNTAIN_FADING,
};

static void PanAndScaleWater(u16 screenX,
u16 screenY,
u16 scaleX,
u16 scaleY,
u16 angle);

// Scene 1 supplemental functions
static void IntroResetGpuRegs(void);
static u8 CreateGameFreakLogoSprites(s16, s16, s16);
static void Task_BlendLogoIn(u8);
static void Task_BlendLogoOut(u8);
static void Task_CreateSparkles(u8);
static u8 CreateWaterDrop(s16, s16, u16, u16, u16, u8);
static void SpriteCB_WaterDrop(struct Sprite *sprite);
static void SpriteCB_WaterDrop_Slide(struct Sprite *);
static void SpriteCB_WaterDrop_ReachLeafEnd(struct Sprite *);
static void SpriteCB_WaterDrop_DangleFromLeaf(struct Sprite *);
static void SpriteCB_WaterDrop_Fall(struct Sprite *);
static void SpriteCB_WaterDrop_Ripple(struct Sprite *);
static void SpriteCB_Sparkle(struct Sprite *sprite);
static void SpriteCB_LogoLetter(struct Sprite *sprite);
static void SpriteCB_GameFreakLogo(struct Sprite *sprite);
static void SpriteCB_FlygonSilhouette(struct Sprite *sprite);

static void Task_Test3_LoadMountainLightning(u8 taskId);
static void Task_Test3_LoadLegend(u8 taskId);
static void Task_Test3_Legend(u8 taskId);
static void Task_Test3_EndLegend(u8 taskId);
static bool8 Test3LightningSpritesAreVisible(struct Task *task);

// Scene 2 main tasks
static void Task_Test2_Wait(u8);
static void Task_Scene2_Load(u8);
static void Task_Scene2_CreateSprites(u8);
static void Task_Scene2_BikeRide(u8);
static void Task_Scene2_End(u8);

// Scene 2 supplemental functions
static void SpriteCB_Torchic(struct Sprite *sprite);
static void SpriteCB_Manectric(struct Sprite *sprite);
static void SpriteCB_Volbeat(struct Sprite *sprite);
static void SpriteCB_Flygon(struct Sprite *);
static void SpriteCB_PlayerOnBicycle(struct Sprite *);

// Scene 3 main tasks
static void Task_Scene3_Load(u8);
static void Task_Scene3_SpinPokeball(u8);
static void Task_Scene3_WaitGroudon(u8);
static void Task_Scene3_LoadGroudon(u8);
static void Task_Scene3_InitGroudonBg(u8);
static void Task_Scene3_NarrowWindow(u8);
static void Task_Scene3_EndNarrowWindow(u8);
static void Task_Scene3_StartGroudon(u8);
static void Task_Scene3_Groudon(u8);
static void Task_Scene3_LoadKyogre(u8);
static void Task_Scene3_Kyogre(u8);
static void Task_Scene3_LoadClouds1(u8);
static void Task_Scene3_LoadClouds2(u8);
static void Task_Scene3_InitClouds(u8);
static void Task_Scene3_Clouds(u8);
static void Task_Scene3_LoadLightning(u8);
static void Task_Scene3_Lightning(u8);
static void Task_Scene3_LoadRayquazaAttack(u8);
static void Task_Scene3_Rayquaza(u8);
static void Task_EndIntroMovie(u8);

// Scene 3 supplemental functions
static void CreateGroudonRockSprites(u8);
static void CreateKyogreBubbleSprites_Body(u8);
static void CreateKyogreBubbleSprites_Fins(void);
static void Task_RayquazaAttack(u8);
static void SpriteCB_GroudonRocks(struct Sprite *);
static void SpriteCB_KyogreBubbles(struct Sprite *sprite);
static void SpriteCB_Lightning(struct Sprite *sprite);
static void SpriteCB_RayquazaOrb(struct Sprite *sprite);

static void MainCB2_EndIntro(void);
static void Task_IntroBackgroundAnimation(u8 taskId);

extern const struct BattleAnimation gBattleAnimTable[ANIM_TAG_COUNT];
extern const struct SpriteTemplate gAncientPowerRockSpriteTemplate;

enum {
COPYRIGHT_INITIALIZE,
COPYRIGHT_EMULATOR_BLEND,
COPYRIGHT_START_FADE = 140,
COPYRIGHT_START_INTRO,
};

#define TAG_VOLBEAT 1500
#define TAG_TORCHIC 1501
#define TAG_MANECTRIC 1502
#define TAG_LIGHTNING 1503
#define TAG_BUBBLES 1504
#define TAG_SPARKLE 1505

#define GFXTAG_DROPS_LOGO 2000
#define PALTAG_DROPS 2000
#define PALTAG_LOGO 2001

#define TAG_FLYGON_SILHOUETTE 2002
#define TAG_RAYQUAZA_ORB 2003

#define COLOSSEUM_GAME_CODE 0x65366347 // "Gc6e" in ASCII

// Used by various tasks and sprites
#define tState data[0]
#define sState data[0]

/*
gIntroFrameCounter is used as a persistent timer throughout the
intro cinematic. At various points it's used to determine when
to trigger actions or progress through the cutscene.
The values for these are defined contiguously below.
*/
#define TIMER_BIG_DROP_START 76
#define TIMER_LOGO_APPEAR 128
#define TIMER_LOGO_LETTERS_COLOR 144
#define TIMER_BIG_DROP_FALLS 251
#define TIMER_LOGO_BLEND_OUT 256
#define TIMER_LOGO_DISAPPEAR 272
#define TIMER_SMALL_DROP_1 368
#define TIMER_SMALL_DROP_2 384
#define TIMER_SPARKLES 560
#define TIMER_FLYGON_SILHOUETTE_APPEAR 832
#define TIMER_END_PAN_UP 904
#define TIMER_END_SCENE_1 1007
#define TIMER_START_SCENE_2 1026
#define TIMER_MANECTRIC_ENTER 1088
#define TIMER_PLAYER_DRIFT_BACK 1109
#define TIMER_MANECTRIC_RUN_CIRCULAR 1168
#define TIMER_PLAYER_MOVE_FORWARD 1214
#define TIMER_TORCHIC_ENTER 1224
#define TIMER_FLYGON_ENTER 1394
#define TIMER_PLAYER_MOVE_BACKWARD 1398
#define TIMER_PLAYER_HOLD_POSITION 1576
#define TIMER_PLAYER_EXIT 1727
#define TIMER_TORCHIC_SPEED_UP 1735
#define TIMER_TORCHIC_EXIT 1856
#define TIMER_END_SCENE_2 1946
#define TIMER_START_SCENE_3 2068
// timer is reset for scene 3
#define TIMER_POKEBALL_FADE 28
#define TIMER_START_LEGENDARIES 43

static EWRAM_DATA enum Gender sIntroCharacterGender = 0;
static EWRAM_DATA u16 sFlygonYOffset = 0;

COMMON_DATA u32 gIntroFrameCounter = 0;
COMMON_DATA struct GcmbStruct gMultibootProgramStruct = {0};

static const u16 sIntroDWorksLogo_Pal[] = INCGFX_U16("graphics/intro/test_1/dworkslogo.png", ".gbapal");
static const u32 sIntroDWorksLogo_Gfx[] = INCGFX_U32("graphics/intro/test_1/dworkslogo.png", ".4bpp.smol");

#define TAG_DWORKS_LOGO 2004
#define PALTAG_DWORKS_LOGO 2004

static const u16 sIntroDrops_Pal[] = INCGFX_U16("graphics/intro/scene_1/drops.pal", ".gbapal");
static const u16 sIntroLogo_Pal[] = INCGFX_U16("graphics/intro/scene_1/logo.pal", ".gbapal");
static const u32 sIntroDropsLogo_Gfx[] = INCGFX_U32("graphics/intro/scene_1/drops_logo.png", ".4bpp.smol");
static const u16 sIntro1Bg_Pal[] = INCGFX_U16("graphics/intro/scene_1/bg.png", ".gbapal"); // 16 x 16
static const u32 sIntro1Bg0_Tilemap[] = INCGFX_U32("graphics/intro/scene_1/bg0_map.bin", ".smolTM");
static const u32 sIntro1Bg1_Tilemap[] = INCGFX_U32("graphics/intro/scene_1/bg1_map.bin", ".smolTM");
static const u32 sIntro1Bg2_Tilemap[] = INCGFX_U32("graphics/intro/scene_1/bg2_map.bin", ".smolTM");
static const u32 sIntro1Bg3_Tilemap[] = INCGFX_U32("graphics/intro/scene_1/bg3_map.bin", ".smolTM");
static const u32 sIntro1Bg_Gfx[] = INCGFX_U32("graphics/intro/scene_1/bg.png", ".4bpp.smol");

static u16 sTest2LoopBgScroll;

// Test 1 ocean affine background.
// These files must be generated as 8bpp graphics with an affine map.
static const u32 sTest_1WaterGfx[] = INCBIN_U32("graphics/intro/test_1/water.img.bin");
static const u16 sTest_1WaterMap[] = INCBIN_U16("graphics/intro/test_1/water.map.bin");
static const u16 sTest_1WaterPal[] = INCBIN_U16("graphics/intro/test_1/water.pal.bin");

// Test 1 mountain text background.
// This is a normal 4bpp text background displayed on BG1.
static const u32 sTest_1MountainGfx[] = INCBIN_U32("graphics/intro/test_1/mounttiles.img.bin");
static const u16 sTest_1MountainMap[] = INCBIN_U16("graphics/intro/test_1/mounttiles.map.bin");
static const u16 sTest_1MountainPal[] = INCBIN_U16("graphics/intro/test_1/mounttiles.pal.bin");

// Minimal Test 3 currently uses only the mountain tiles, map, and palette.
static const u32 sTest3MountainGfx[] = INCBIN_U32("graphics/intro/test_3/mounttiles.img.bin");
static const u16 sTest3MountainMap[] = INCBIN_U16("graphics/intro/test_3/mounttiles.map.bin");
static const u16 sTest3MountainPal[] = INCBIN_U16("graphics/intro/test_3/mounttiles.pal.bin");

static const u32 sLegendBgGfx[] = INCBIN_U32("graphics/intro/test_3/legend_bg.img.bin");
static const u16 sLegendBgMap0[] = INCBIN_U16("graphics/intro/test_3/0.map.bin");
static const u16 sLegendBgMap1[] = INCBIN_U16("graphics/intro/test_3/1.map.bin");
static const u16 sLegendBgPal[] = INCBIN_U16("graphics/intro/test_3/legend_bg.pal.bin");

/* Test 3 palette state: grey silhouette between the original lightning strikes. */
static u16 sTest3MountainOriginalPalette[16];
static u16 sTest3MountainMutedPalette[16];
static u8 sTest3MountainLightningMode;

// Shared 4bpp tile graphics and palette, plus one tilemap per intro image.
// Generate these with Grit using ../../../../grit/grit 0.png 1.png 2.png 3.png 4.png 5.png -gB4 -gS -pS -m -mR4 -ftb -O test_bg
static const u32 sTestBgSharedGfx[] = INCBIN_U32("graphics/intro/test_2/test_bg.img.bin");
static const u16 sTestBgSharedPal[] = INCBIN_U16("graphics/intro/test_2/test_bg.pal.bin");
static const u16 sTestBg0Tilemap[] = INCBIN_U16("graphics/intro/test_2/0.map.bin");
static const u16 sTestBg1Tilemap[] = INCBIN_U16("graphics/intro/test_2/1.map.bin");
static const u16 sTestBg2Tilemap[] = INCBIN_U16("graphics/intro/test_2/2.map.bin");
static const u16 sTestBg3Tilemap[] = INCBIN_U16("graphics/intro/test_2/3.map.bin");
static const u16 sTestBg4Tilemap[] = INCBIN_U16("graphics/intro/test_2/4.map.bin");
static const u16 sTestBg5Tilemap[] = INCBIN_U16("graphics/intro/test_2/5.map.bin");

static const u32 sTest2LoopBgGfx[] =
    INCBIN_U32("graphics/intro/test_2/loop_bg.img.bin");

static const u16 sTest2LoopBgPal[] =
    INCBIN_U16("graphics/intro/test_2/loop_bg.pal.bin");

static const u16 sTest2LoopBgTilemap[] =
    INCBIN_U16("graphics/intro/test_2/loop_bg.map.bin");

static const u32 sTest2FireBgGfx[] =
    INCBIN_U32("graphics/intro/test_2/firebg.img.bin");

static const u16 sTest2FireBgTilemap[] =
    INCBIN_U16("graphics/intro/test_2/firebg.map.bin");

static const u16 sTest2FireBgPal[] =
    INCBIN_U16("graphics/intro/test_2/firebg.pal.bin");

static const u32 sTest2WaterBgGfx[] =
    INCBIN_U32("graphics/intro/test_2/waterbg.img.bin");

static const u16 sTest2WaterBgTilemap[] =
    INCBIN_U16("graphics/intro/test_2/waterbg.map.bin");

static const u16 sTest2WaterBgPal[] =
    INCBIN_U16("graphics/intro/test_2/waterbg.pal.bin");

static const u32 sTest2GrassBgGfx[] =
    INCBIN_U32("graphics/intro/test_2/grassbg.img.bin");

static const u16 sTest2GrassBgTilemap[] =
    INCBIN_U16("graphics/intro/test_2/grassbg.map.bin");

static const u16 sTest2GrassBgPal[] =
    INCBIN_U16("graphics/intro/test_2/grassbg.pal.bin");

static const u16 sIntroPokeball_Pal[] = INCGFX_U16("graphics/intro/scene_3/pokeball.png", ".gbapal");
static const u32 sIntroPokeball_Tilemap[] = INCGFX_U32("graphics/intro/scene_3/pokeball_map.bin", ".smolTM");
static const u32 sIntroPokeball_Gfx[] = INCGFX_U32("graphics/intro/scene_3/pokeball.png", ".8bpp.smol");
static const u16 sIntroStreaks_Pal[] = INCGFX_U16("graphics/intro/scene_3/streaks.png", ".gbapal"); // Unused
static const u32 sIntroStreaks_Gfx[] = INCGFX_U32("graphics/intro/scene_3/streaks.png", ".4bpp.smol"); // Unused
static const u32 sIntroStreaks_Tilemap[] = INCGFX_U32("graphics/intro/scene_3/streaks_map.bin", ".smolTM"); // Unused
static const u16 sIntroRayquzaOrb_Pal[] = INCGFX_U16("graphics/intro/scene_3/rayquaza_orb.pal", ".gbapal");
static const u16 sIntroMisc_Pal[] = INCGFX_U16("graphics/intro/scene_3/misc.pal", ".gbapal"); // Unused
static const u32 sIntroMisc_Gfx[] = INCGFX_U32("graphics/intro/scene_3/misc.png", ".4bpp.smol"); // Rayquza orb, and misc unused gfx
static const u16 sIntroFlygonSilhouette_Pal[] = INCGFX_U16("graphics/intro/scene_1/flygon.png", ".gbapal");
static const u32 sIntroLati_Gfx[] = INCGFX_U32("graphics/intro/scene_1/lati.png", ".4bpp.smol"); // Unused
static const u8 sUnusedData[] = {
0x02, 0x03, 0x04, 0x05, 0x01, 0x01, 0x01, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x02, 0x0D,
0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x02, 0x0D, 0x0E, 0x0F,
0x10, 0x11, 0x12, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x02, 0x0D, 0x0E, 0x0F, 0x10,
0x11, 0x12, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x00
};

static const struct CompressedSpriteSheet sSpriteSheet_DWorksLogo =
{
    sIntroDWorksLogo_Gfx, 0x800, TAG_DWORKS_LOGO,
};

static const struct SpritePalette sSpritePalette_DWorksLogo =
{
    sIntroDWorksLogo_Pal, PALTAG_DWORKS_LOGO,
};

static const struct OamData sOamData_DWorksLogo =
{
    .y = DISPLAY_HEIGHT,
    .affineMode = ST_OAM_AFFINE_OFF,
    .objMode = ST_OAM_OBJ_BLEND, // Uses blend mode so it can fade out smoothly
    .mosaic = FALSE,
    .bpp = ST_OAM_4BPP,
    .shape = SPRITE_SHAPE(64x64),
    .x = 0,
    .matrixNum = 0,
    .size = SPRITE_SIZE(64x64),
    .tileNum = 0,
    .priority = 0,
    .paletteNum = 0,
    .affineParam = 0,
};

static const union AnimCmd sAnim_DWorksLogo[] =
{
    ANIMCMD_FRAME(0, 8),
    ANIMCMD_END,
};

static const union AnimCmd *const sAnims_DWorksLogo[] =
{
    sAnim_DWorksLogo,
};

static void SpriteCB_DWorksLogo(struct Sprite *sprite);

static const struct SpriteTemplate sSpriteTemplate_DWorksLogo =
{
    .tileTag = TAG_DWORKS_LOGO,
    .paletteTag = PALTAG_DWORKS_LOGO,
    .oam = &sOamData_DWorksLogo,
    .anims = sAnims_DWorksLogo,
    .callback = SpriteCB_DWorksLogo,
};

static const struct CompressedSpriteSheet sSpriteSheet_Sparkle[] =
{
{gIntroSparkle_Gfx, 0x400, TAG_SPARKLE},
{},
};
static const struct SpritePalette sSpritePalette_Sparkle[] =
{
{gIntroLightning_Pal, TAG_SPARKLE}, // Lightning palette re-used
{},
};
static const struct OamData sOamData_Sparkle =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(16x16),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(16x16),
.tileNum = 0,
.priority = 1,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Sparkle[] =
{
ANIMCMD_FRAME(0, 2),
ANIMCMD_FRAME(4, 2),
ANIMCMD_FRAME(8, 2),
ANIMCMD_FRAME(12, 2),
ANIMCMD_FRAME(16, 2),
ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Sparkle[] =
{
sAnim_Sparkle,
};
static const struct SpriteTemplate sSpriteTemplate_Sparkle =
{
.tileTag = TAG_SPARKLE,
.paletteTag = TAG_SPARKLE,
.oam = &sOamData_Sparkle,
.anims = sAnims_Sparkle,
.callback = SpriteCB_Sparkle,
};
static const u8 sSparkleCoords[][2] =
{
{124, 40},
{102, 30},
{ 77, 30},
{ 54, 15},
{148, 9},
{ 63, 28},
{ 93, 40},
{148, 32},
{173, 41},
{ 94, 20},
{208, 38},
{},
};
static const struct CompressedSpriteSheet sSpriteSheet_RunningPokemon[] =
{
{gIntroVolbeat_Gfx, 0x400, TAG_VOLBEAT},
{gIntroTorchic_Gfx, 0xC00, TAG_TORCHIC},
{gIntroManectric_Gfx, 0x2000, TAG_MANECTRIC},
{},
};
static const struct SpritePalette sSpritePalettes_RunningPokemon[] =
{
{gIntroVolbeat_Pal, TAG_VOLBEAT},
{gIntroTorchic_Pal, TAG_TORCHIC},
{gIntroManectric_Pal, TAG_MANECTRIC},
{},
};
static const struct OamData sOamData_Volbeat =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(32x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(32x32),
.tileNum = 0,
.priority = 1,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Volbeat[] =
{
ANIMCMD_FRAME(0, 2),
ANIMCMD_FRAME(16, 2),
ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Volbeat[] =
{
sAnim_Volbeat,
};
static const struct SpriteTemplate sSpriteTemplate_Volbeat =
{
.tileTag = TAG_VOLBEAT,
.paletteTag = TAG_VOLBEAT,
.oam = &sOamData_Volbeat,
.anims = sAnims_Volbeat,
.callback = SpriteCB_Volbeat,
};
static const struct OamData sOamData_Torchic =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(32x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(32x32),
.tileNum = 0,
.priority = 1,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Torchic_Walk[] =
{
ANIMCMD_FRAME(0, 5),
ANIMCMD_FRAME(16, 5),
ANIMCMD_FRAME(32, 5),
ANIMCMD_FRAME(16, 5),
ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_Torchic_Run[] =
{
ANIMCMD_FRAME(0, 3),
ANIMCMD_FRAME(16, 3),
ANIMCMD_FRAME(32, 3),
ANIMCMD_FRAME(16, 3),
ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_Torchic_Trip[] =
{
ANIMCMD_FRAME(48, 4),
ANIMCMD_FRAME(64, 6),
ANIMCMD_FRAME(80, 0),
ANIMCMD_END,
};
enum {
TORCHIC_ANIM_WALK,
TORCHIC_ANIM_RUN,
TORCHIC_ANIM_TRIP,
};
static const union AnimCmd *const sAnims_Torchic[] =
{
[TORCHIC_ANIM_WALK] = sAnim_Torchic_Walk,
[TORCHIC_ANIM_RUN] = sAnim_Torchic_Run,
[TORCHIC_ANIM_TRIP] = sAnim_Torchic_Trip,
};
static const struct SpriteTemplate sSpriteTemplate_Torchic =
{
.tileTag = TAG_TORCHIC,
.paletteTag = TAG_TORCHIC,
.oam = &sOamData_Torchic,
.anims = sAnims_Torchic,
.callback = SpriteCB_Torchic,
};
static const struct OamData sOamData_Manectric =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(64x64),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(64x64),
.tileNum = 0,
.priority = 1,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Manectric[] =
{
ANIMCMD_FRAME(0, 4),
ANIMCMD_FRAME(64, 4),
ANIMCMD_FRAME(128, 4),
ANIMCMD_FRAME(192, 4),
ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_Manectric[] =
{
sAnim_Manectric,
};
static const struct SpriteTemplate sSpriteTemplate_Manectric =
{
.tileTag = TAG_MANECTRIC,
.paletteTag = TAG_MANECTRIC,
.oam = &sOamData_Manectric,
.anims = sAnims_Manectric,
.callback = SpriteCB_Manectric,
};
static const struct CompressedSpriteSheet sSpriteSheet_Lightning[] =
{
{gIntroLightning_Gfx, 0xC00, TAG_LIGHTNING},
{},
};
static const struct SpritePalette sSpritePalette_Lightning[] =
{
{gIntroLightning_Pal, TAG_LIGHTNING},
{},
};
static const struct OamData sOamData_Lightning =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(32x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(32x32),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Lightning_Top[] =
{
ANIMCMD_FRAME(0, 2),
ANIMCMD_FRAME(48, 2),
ANIMCMD_END,
};
static const union AnimCmd sAnim_Lightning_Middle[] =
{
ANIMCMD_FRAME(16, 2),
ANIMCMD_FRAME(64, 2),
ANIMCMD_END,
};
static const union AnimCmd sAnim_Lightning_Bottom[] =
{
ANIMCMD_FRAME(32, 2),
ANIMCMD_FRAME(80, 2),
ANIMCMD_END,
};
static const union AnimCmd *const sAnims_Lightning[] =
{
sAnim_Lightning_Top,
sAnim_Lightning_Middle,
sAnim_Lightning_Bottom,
};
static const struct SpriteTemplate sSpriteTemplate_Lightning =
{
.tileTag = TAG_LIGHTNING,
.paletteTag = TAG_LIGHTNING,
.oam = &sOamData_Lightning,
.anims = sAnims_Lightning,
.callback = SpriteCB_Lightning,
};
// x coord, anim number, speed
// Smaller anim numbers are larger rocks, and are given slower speeds
static const s16 sGroudonRockData[][3] =
{
{104, 0, 0x0C0},
{142, 3, 0x280},
{ 83, 1, 0x180},
{155, 0, 0x080},
{ 56, 2, 0x200},
{174, 1, 0x100},
};
static const struct CompressedSpriteSheet sSpriteSheet_Bubbles[] =
{
{gIntroBubbles_Gfx, 0x600, TAG_BUBBLES},
{},
};
static const struct SpritePalette sSpritePalette_Bubbles[] =
{
{gIntroBubbles_Pal, TAG_BUBBLES},
{},
};
#define NUM_BUBBLES_IN_SET 6
// x coord, y coord, delay before animation
// Can be produced in two different sets depending on the function called to create the sprites
static const s16 sKyogreBubbleData[NUM_BUBBLES_IN_SET * 2][3] =
{
// Set 1, for Kyogre's body
{ 66, 64, 1},
{ 96, 96, 8},
{128, 64, 1},
{144, 48, 8},
{160, 72, 1},
{176, 96, 8},
// Set 2, for Kyogre's fins
{ 96, 96, 4},
{112, 104, 8},
{128, 96, 4},
{ 88, 32, 4},
{104, 24, 8},
{120, 32, 4},
};
static const struct OamData sOamData_Bubbles =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(16x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(16x32),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_Bubbles[] =
{
ANIMCMD_FRAME(0, 4),
ANIMCMD_FRAME(8, 4),
ANIMCMD_FRAME(16, 4),
ANIMCMD_FRAME(24, 4),
ANIMCMD_FRAME(32, 4),
ANIMCMD_END,
};
static const union AnimCmd *const sAnims_Bubbles[] =
{
sAnim_Bubbles,
};
static const struct SpriteTemplate sSpriteTemplate_Bubbles =
{
.tileTag = TAG_BUBBLES,
.paletteTag = TAG_BUBBLES,
.oam = &sOamData_Bubbles,
.anims = sAnims_Bubbles,
.callback = SpriteCB_KyogreBubbles,
};
static const struct OamData sOamData_WaterDrop =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(32x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(32x32),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
enum {
DROP_ANIM_UPPER_HALF,
DROP_ANIM_LOWER_HALF,
DROP_ANIM_REFLECTION,
DROP_ANIM_RIPPLE,
};
static const union AnimCmd sAnim_WaterDrop_UpperHalf[] =
{
ANIMCMD_FRAME(16, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_LowerHalf[] =
{
ANIMCMD_FRAME(24, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_Reflection[] =
{
ANIMCMD_FRAME(0, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_WaterDrop_Ripple[] =
{
ANIMCMD_FRAME(48, 8),
ANIMCMD_END,
};
static const union AnimCmd *const sAnims_WaterDrop[] =
{
[DROP_ANIM_UPPER_HALF] = sAnim_WaterDrop_UpperHalf,
[DROP_ANIM_LOWER_HALF] = sAnim_WaterDrop_LowerHalf,
[DROP_ANIM_REFLECTION] = sAnim_WaterDrop_Reflection,
[DROP_ANIM_RIPPLE] = sAnim_WaterDrop_Ripple,
};
static const struct SpriteTemplate sSpriteTemplate_WaterDrop =
{
.tileTag = GFXTAG_DROPS_LOGO,
.paletteTag = PALTAG_DROPS,
.oam = &sOamData_WaterDrop,
.anims = sAnims_WaterDrop,
.callback = SpriteCB_WaterDrop,
};
static const union AnimCmd sAnim_PlayerBicycle_Fast[] =
{
ANIMCMD_FRAME(0, 4),
ANIMCMD_FRAME(64, 4),
ANIMCMD_FRAME(128, 4),
ANIMCMD_FRAME(192, 4),
ANIMCMD_JUMP(0),
};
static const union AnimCmd sAnim_PlayerBicycle_Slow[] =
{
ANIMCMD_FRAME(0, 8),
ANIMCMD_FRAME(64, 8),
ANIMCMD_FRAME(128, 8),
ANIMCMD_FRAME(192, 8),
ANIMCMD_JUMP(0),
};
// The below two animations appear to be copied from the Credits version
// of the player graphic, where additional frames are present to show
// the player turning around to look at their rival.
// They go unused here, and if they were used they'd overflow beyond
// the player graphics data.
// The above sAnim_PlayerBicycle_Slow, while valid, is likewise unused
static const union AnimCmd sAnim_PlayerBicycle_LookBack[] =
{
ANIMCMD_FRAME(256, 4),
ANIMCMD_FRAME(320, 4),
ANIMCMD_FRAME(384, 4),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PlayerBicycle_LookForward[] =
{
ANIMCMD_FRAME(384, 16),
ANIMCMD_FRAME(320, 16),
ANIMCMD_FRAME(256, 16),
ANIMCMD_END,
};
static const union AnimCmd *const sAnims_PlayerBicycle[] =
{
sAnim_PlayerBicycle_Fast,
sAnim_PlayerBicycle_Slow,
sAnim_PlayerBicycle_LookBack,
sAnim_PlayerBicycle_LookForward,
};
static const struct OamData sOamData_GameFreakLetter =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_DOUBLE,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(16x16),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(16x16),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const struct OamData sOamData_PresentsLetter =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(8x8),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(8x8),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const struct OamData sOamData_GameFreakLogo =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_DOUBLE,
.objMode = ST_OAM_OBJ_BLEND,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(32x64),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(32x64),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_GameFreakLetter_G[] =
{
ANIMCMD_FRAME(80, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_A[] =
{
ANIMCMD_FRAME(84, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_M[] =
{
ANIMCMD_FRAME(88, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_E[] =
{
ANIMCMD_FRAME(92, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_F[] =
{
ANIMCMD_FRAME(96, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_R[] =
{
ANIMCMD_FRAME(100, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLetter_K[] =
{
ANIMCMD_FRAME(104, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_P[] =
{
ANIMCMD_FRAME(112, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_R[] =
{
ANIMCMD_FRAME(113, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_E[] =
{
ANIMCMD_FRAME(114, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_S[] =
{
ANIMCMD_FRAME(115, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_N[] =
{
ANIMCMD_FRAME(116, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_PresentsLetter_T[] =
{
ANIMCMD_FRAME(117, 8),
ANIMCMD_END,
};
static const union AnimCmd sAnim_GameFreakLogo[] =
{
ANIMCMD_FRAME(128, 8),
ANIMCMD_END,
};

enum {
GAMEFREAK_G,
GAMEFREAK_A,
GAMEFREAK_M,
GAMEFREAK_E,
GAMEFREAK_F,
GAMEFREAK_R,
GAMEFREAK_K,
};
enum {
PRESENTS_P,
PRESENTS_R,
PRESENTS_E,
PRESENTS_S,
PRESENTS_N,
PRESENTS_T,
};
static const union AnimCmd *const sAnims_GameFreakLetter[] =
{
[GAMEFREAK_G] = sAnim_GameFreakLetter_G,
[GAMEFREAK_A] = sAnim_GameFreakLetter_A,
[GAMEFREAK_M] = sAnim_GameFreakLetter_M,
[GAMEFREAK_E] = sAnim_GameFreakLetter_E,
[GAMEFREAK_F] = sAnim_GameFreakLetter_F,
[GAMEFREAK_R] = sAnim_GameFreakLetter_R,
[GAMEFREAK_K] = sAnim_GameFreakLetter_K,
};
static const union AnimCmd *const sAnims_PresentsLetter[] =
{
[PRESENTS_P] = sAnim_PresentsLetter_P,
[PRESENTS_R] = sAnim_PresentsLetter_R,
[PRESENTS_E] = sAnim_PresentsLetter_E,
[PRESENTS_S] = sAnim_PresentsLetter_S,
[PRESENTS_N] = sAnim_PresentsLetter_N,
[PRESENTS_T] = sAnim_PresentsLetter_T,
};
static const union AnimCmd *const sAnims_GameFreakLogo[] =
{
sAnim_GameFreakLogo,
};
#define NUM_GF_LETTERS 9 // Letters in "Game Freak"
static const s16 sGameFreakLetterData[NUM_GF_LETTERS][2] =
{
// Letter, x offset
{GAMEFREAK_G, -72},
{GAMEFREAK_A, -56},
{GAMEFREAK_M, -40},
{GAMEFREAK_E, -24},
{GAMEFREAK_F, 8},
{GAMEFREAK_R, 24},
{GAMEFREAK_E, 40},
{GAMEFREAK_A, 56},
{GAMEFREAK_K, 72},
};
static const s16 sPresentsLetterData[][2] =
{
// Letter, x offset
{PRESENTS_P, -28},
{PRESENTS_R, -20},
{PRESENTS_E, -12},
{PRESENTS_S, -4},
{PRESENTS_E, 4},
{PRESENTS_N, 12},
{PRESENTS_T, 20},
{PRESENTS_S, 28},
};
static const union AffineAnimCmd sAffineAnim_GameFreak_Small[] =
{
AFFINEANIMCMD_FRAME(128, 128, 0, 0),
AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowAndShrink[] =
{
AFFINEANIMCMD_FRAME(128, 128, 0, 0),
AFFINEANIMCMD_FRAME(16, 16, 0, 16),
AFFINEANIMCMD_FRAME(-16, -16, 0, 8),
AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowBig[] =
{
AFFINEANIMCMD_FRAME(256, 256, 0, 0),
AFFINEANIMCMD_FRAME(8, 8, 0, 48),
AFFINEANIMCMD_END,
};
static const union AffineAnimCmd sAffineAnim_GameFreak_GrowMedium[] =
{
AFFINEANIMCMD_FRAME(256, 256, 0, 0),
AFFINEANIMCMD_FRAME(2, 2, 0, 48),
AFFINEANIMCMD_END,
};
static const union AffineAnimCmd *const sAffineAnims_GameFreak[] =
{
sAffineAnim_GameFreak_Small, // Initialize letters while still invisible
sAffineAnim_GameFreak_GrowAndShrink, // For letters appearing. Logo does this too, but while it's invisible
sAffineAnim_GameFreak_GrowBig, // For letters disappearing
sAffineAnim_GameFreak_GrowMedium, // For logo disappearing
};
static const u16 sGameFreakLettersMoveSpeed[NUM_GF_LETTERS] =
{
256, // G
192, // A
128, // M
64, // E
0, // F
64, // R
128, // E
192, // A
256 // K
};
static const struct SpriteTemplate sSpriteTemplate_GameFreakLetter =
{
.tileTag = GFXTAG_DROPS_LOGO,
.paletteTag = PALTAG_LOGO,
.oam = &sOamData_GameFreakLetter,
.anims = sAnims_GameFreakLetter,
.affineAnims = sAffineAnims_GameFreak,
.callback = SpriteCB_LogoLetter,
};
// Unused
static const struct SpriteTemplate sSpriteTemplate_PresentsLetter =
{
.tileTag = GFXTAG_DROPS_LOGO,
.paletteTag = PALTAG_LOGO,
.oam = &sOamData_PresentsLetter,
.anims = sAnims_PresentsLetter,
.callback = SpriteCB_LogoLetter,
};
static const struct SpriteTemplate sSpriteTemplate_GameFreakLogo =
{
.tileTag = GFXTAG_DROPS_LOGO,
.paletteTag = PALTAG_LOGO,
.oam = &sOamData_GameFreakLogo,
.anims = sAnims_GameFreakLogo,
.affineAnims = sAffineAnims_GameFreak,
.callback = SpriteCB_GameFreakLogo,
};
static const u8 sGameFreakLetterStartDelays[NUM_GF_LETTERS] =
{
0, // G
23, // A
23, // M
49, // E
62, // F
36, // R
36, // E
10, // A
10 // K
};
static const struct OamData sOamData_FlygonSilhouette =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(64x32),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(64x32),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_FlygonSilhouette[] =
{
ANIMCMD_FRAME(0, 10),
ANIMCMD_JUMP(0),
};
static const union AnimCmd *const sAnims_FlygonSilhouette[] =
{
sAnim_FlygonSilhouette,
};
static const struct SpriteTemplate sSpriteTemplate_FlygonSilhouette =
{
.tileTag = TAG_FLYGON_SILHOUETTE,
.paletteTag = TAG_FLYGON_SILHOUETTE,
.oam = &sOamData_FlygonSilhouette,
.anims = sAnims_FlygonSilhouette,
.callback = SpriteCB_FlygonSilhouette,
};
static const struct CompressedSpriteSheet sSpriteSheet_WaterDropsAndLogo[] =
{
{sIntroDropsLogo_Gfx, 0x1400, GFXTAG_DROPS_LOGO},
{},
};
static const struct CompressedSpriteSheet sSpriteSheet_FlygonSilhouette[] =
{
{gIntroFlygonSilhouette_Gfx, 0x400, TAG_FLYGON_SILHOUETTE},
{},
};
static const struct SpritePalette sSpritePalettes_Intro1[] =
{
{sIntroDrops_Pal, PALTAG_DROPS},
{sIntroLogo_Pal, PALTAG_LOGO},
{sIntroFlygonSilhouette_Pal, TAG_FLYGON_SILHOUETTE},
{},
};
static const struct OamData sOamData_RayquazaOrb =
{
.y = DISPLAY_HEIGHT,
.affineMode = ST_OAM_AFFINE_OFF,
.objMode = ST_OAM_OBJ_NORMAL,
.mosaic = FALSE,
.bpp = ST_OAM_4BPP,
.shape = SPRITE_SHAPE(64x64),
.x = 0,
.matrixNum = 0,
.size = SPRITE_SIZE(64x64),
.tileNum = 0,
.priority = 0,
.paletteNum = 0,
.affineParam = 0,
};
static const union AnimCmd sAnim_RayquazaOrb[] =
{
ANIMCMD_FRAME(16, 8),
ANIMCMD_END,
};
static const union AnimCmd *const sAnims_RayquazaOrb[] =
{
sAnim_RayquazaOrb,
};
static const struct SpriteTemplate sSpriteTemplate_RayquazaOrb =
{
.tileTag = TAG_RAYQUAZA_ORB,
.paletteTag = TAG_RAYQUAZA_ORB,
.oam = &sOamData_RayquazaOrb,
.anims = sAnims_RayquazaOrb,
.callback = SpriteCB_RayquazaOrb,
};
static const struct CompressedSpriteSheet sSpriteSheet_RayquazaOrb[] =
{
{sIntroMisc_Gfx, 0xA00, TAG_RAYQUAZA_ORB},
{},
};
static const struct SpritePalette sSpritePalette_RayquazaOrb[] =
{
{sIntroRayquzaOrb_Pal, TAG_RAYQUAZA_ORB},
{},
};

static const u16 *const sTest2BgTilemaps[] =
{
sTestBg0Tilemap,
sTestBg1Tilemap,
sTestBg2Tilemap,
sTestBg3Tilemap,
sTestBg4Tilemap,
sTestBg5Tilemap,
};

#define TEST1_LOGO_DEBUG_HOLD_END 392
#define TEST1_CAMERA_PAN_END 512
#define TEST1_PERSPECTIVE_FRAMES 120
#define TEST1_HORIZON_Y 104 // Final water occupies the bottom 56 lines (~35% of screen)
#define TEST1_WATER_SCROLL_SPEED 1
#define TEST1_CAMERA_HEIGHT 96

static u16 sIntroWaterScroll;

#define TEST1_MOUNTAIN_CHARBASE 2
#define TEST1_MOUNTAIN_SCREENBASE 28
#define TEST1_MOUNTAIN_PALETTE 1

#define TEST1_MOUNTAIN_INITIAL_VOFS TEST1_HORIZON_Y // Pan from 104 pixels to 0
#define TEST1_MOUNTAIN_HOLD_FRAMES 60
#define TEST1_MOUNTAIN_FADE_FRAMES 16

#define TEST3_LIGHTNING_FIRST_DELAY  1
#define TEST3_LIGHTNING_SECOND_DELAY 72
#define TEST3_LIGHTNING_FADE_DELAY   48
#define TEST3_MOUNTAIN_GRAY_FADE     16

/*
 * Test 2 uses 4bpp text backgrounds. BG character data and screen maps
 * share the same 64KB of BG VRAM: character blocks are 16KB units and
 * screen blocks are 2KB units.
 *
 * Character data layout:
 *   charbase 0, byte 0:                 fire background
 *   immediately after fire:            runner/test background
 *   charbase 2, byte 0:                 water background
 *   immediately after water:           grass background
 *   immediately after grass:           looping background
 *
 * The sizes in this layout are tile-aligned (32 bytes per 4bpp tile).
 * The complete character-data region ends before screen block 28, where
 * the loop map lives. Screen block 30 is shared by the runner and mon
 * maps because those scenes are mutually exclusive.
 */
#define TEST2_LOOP_BG_CHARBASE       2
#define TEST2_LOOP_BG_SCREENBASE     28
#define TEST2_LOOP_BG_PALETTE        1
#define TEST2_SCENE_SCREENBASE       30
#define TEST2_BG_SCROLL_SPEED       5

#define TEST2_RUNNER_TILE_OFFSET     (sizeof(sTest2FireBgGfx) / 32)
#define TEST2_GRASS_TILE_OFFSET      (sizeof(sTest2WaterBgGfx) / 32)
#define TEST2_LOOP_TILE_OFFSET       ((sizeof(sTest2WaterBgGfx) + sizeof(sTest2GrassBgGfx)) / 32)

/* Fail the build rather than silently overlap graphics or screen maps. */
typedef char test2_charbase0_fits[
    (sizeof(sTest2FireBgGfx) + sizeof(sTestBgSharedGfx) <= 0x8000) ? 1 : -1
];
typedef char test2_charbase2_fits[
    (sizeof(sTest2WaterBgGfx) + sizeof(sTest2GrassBgGfx)
        + sizeof(sTest2LoopBgGfx) <= 0x6000) ? 1 : -1
];
typedef char test2_element_maps_are_32x32[
    (sizeof(sTest2FireBgTilemap) == 0x800
        && sizeof(sTest2WaterBgTilemap) == 0x800
        && sizeof(sTest2GrassBgTilemap) == 0x800
        && sizeof(sTest2LoopBgTilemap) == 0x800) ? 1 : -1
];
typedef char test2_element_palettes_are_16_bank[
    (sizeof(sTest2FireBgPal) == PLTT_SIZE_4BPP * 16
        && sizeof(sTest2WaterBgPal) == PLTT_SIZE_4BPP * 16
        && sizeof(sTest2GrassBgPal) == PLTT_SIZE_4BPP * 16) ? 1 : -1
];

#define TEST2_CINEMATIC_BAR_HEIGHT       48
#define TEST2_PAN_DURATION_SECS          10  // 10 seconds duration
#define TEST2_PAN_FRAMES                 (TEST2_PAN_DURATION_SECS * 60)
#define TEST2_TOP_HOLD_FRAMES            40
#define TEST2_BAR_RETRACT_FRAMES         48
#define TEST2_OPEN_RUN_FRAMES            160

#define TEST2_START_VOFS                 48          // Start 48 pixels north
#define TEST2_END_VOFS                   (-48)       // End 48 pixels south
#define TEST2_TOTAL_VOFS_DISTANCE        (TEST2_START_VOFS - TEST2_END_VOFS) // 96 px

#define TEST2_MON_INTERVAL_FRAMES (TEST2_PAN_FRAMES / 6)
#define TEST2_MON_HOLD_FRAMES     96
#define TEST2_MON_X               (DISPLAY_WIDTH / 2)
#define TEST2_MON_Y               (DISPLAY_HEIGHT / 2)

enum
{
    TEST2_MON_NONE,
    TEST2_MON_CINDERF,
    TEST2_MON_NEREQUA,
    TEST2_MON_FEVINE,
};

enum
{
    TEST2_MON_STATE_NONE,
    TEST2_MON_STATE_LOAD_BACKGROUND,
    TEST2_MON_STATE_RELEASE,
    TEST2_MON_STATE_HOLD,
    TEST2_MON_STATE_CRY,
    TEST2_MON_STATE_RETURN,
};

struct Test2MonData
{
    u16 species;
    const u16 *backgroundTilemap;
    const u16 *backgroundPal;
    u8 backgroundCharbase;
    u16 backgroundTileOffset;
};

static const struct Test2MonData sTest2MonData[] =
{
    [TEST2_MON_CINDERF] =
    {
        SPECIES_CINDERF,
        sTest2FireBgTilemap,
        sTest2FireBgPal,
        0,
        0,
    },
    [TEST2_MON_NEREQUA] =
    {
        SPECIES_NEREQUA,
        sTest2WaterBgTilemap,
        sTest2WaterBgPal,
        2,
        0,
    },
    [TEST2_MON_FEVINE] =
    {
        SPECIES_FEVINE,
        sTest2GrassBgTilemap,
        sTest2GrassBgPal,
        2,
        TEST2_GRASS_TILE_OFFSET,
    },
};

#define tTest2MonNumber       data[5]
#define tTest2MonState        data[6]
#define tTest2MonTimer        data[7]
#define tTest2MonSpriteId     data[8]
#define tTest2VolbeatSpriteId data[9]
#define tTest2TorchicSpriteId data[10]
#define tTest2ManectricSpriteId data[11]
#define tTest2MonCryPlayed    data[12]
#define tTest2NextMonFrame data[13]

enum
{
    TEST2_STATE_PAN = 0,
    TEST2_STATE_TOP_HOLD,
    TEST2_STATE_RETRACT_BARS,
    TEST2_STATE_OPEN_RUN,
    TEST2_STATE_FADE_TO_BLACK,
    TEST2_STATE_WAIT_FOR_FADE,
};

#define tTest2State       data[2]
#define tTest2Timer       data[3]
#define tTest2Displacement data[4]

static u16 sTest2SharedDisplacement;
static u8 sTest2BarHeight;

#define tTest1WaterFrame data[1]
#define tTest1WaterScroll data[2]
#define tTest1MountainState data[3]
#define tTest1MountainHold data[4]

#define INTRO_BG_TILEMAP_WIDTH 30
#define INTRO_BG_TILEMAP_HEIGHT 20
#define INTRO_BG_SCREEN_WIDTH 32
#define INTRO_BG_SCREEN_HEIGHT 32

static void CopyTest_1WaterMap(void)
{
CpuCopy16(sTest_1WaterMap,
(void *)BG_SCREEN_ADDR(30),
sizeof(sTest_1WaterMap));
}

static void CopyTest_1MountainMap(void)
{
    const u16 *src = sTest_1MountainMap;
    u16 *dst = (u16 *)BG_SCREEN_ADDR(TEST1_MOUNTAIN_SCREENBASE);
    u16 row;
    u16 col;

    // The padded mountain source/map is 256x256: 32x32 tiles.
    // Copy using the GBA screen-block stride (32 tiles), not the
    // 30-tile stride used by the 240x160 test_2 images. A 30-tile
    // source stride shifts each successive row by 2 tiles (16 pixels).
    for (row = 0; row < INTRO_BG_SCREEN_HEIGHT; row++)
    {
        for (col = 0; col < INTRO_BG_SCREEN_WIDTH; col++)
        {
            // Preserve tile index and flip bits, but select palette bank 1.
            dst[row * INTRO_BG_SCREEN_WIDTH + col] =
                src[row * INTRO_BG_SCREEN_WIDTH + col]
                | (TEST1_MOUNTAIN_PALETTE << 12);
        }
    }
}

struct IntroBgAnimFrame
{
u8 background;
u8 duration;
};

static const struct IntroBgAnimFrame sIntroBgAnimation[] =
{
{0, 10},
{1, 5},
{2, 5},
{3, 10},
{4, 5},
{5, 5},
};

#define tIntroBgFrame data[0]
#define tIntroBgTimer data[1]

static u8 sIntroPendingTilemap;
static bool8 sIntroTilemapPending;

static void CopyTest2IntroTilemap(u8 background)
{
    const u16 *src = sTest2BgTilemaps[background];
    u16 *dst = (u16 *)BG_SCREEN_ADDR(TEST2_SCENE_SCREENBASE);
    u8 row;
    u8 col;

    /*
     * These six runner maps are 30x20 (240x160) images. Their tile data
     * begins after the fire tiles in charbase 0, so add that tile offset
     * while preserving flip and palette-bank bits.
     */
    for (row = 0; row < INTRO_BG_TILEMAP_HEIGHT; row++)
    {
        for (col = 0; col < INTRO_BG_TILEMAP_WIDTH; col++)
        {
            u16 tile = src[row * INTRO_BG_TILEMAP_WIDTH + col];
            u16 tileIndex = (tile & 0x03FF) + TEST2_RUNNER_TILE_OFFSET;

            dst[row * INTRO_BG_SCREEN_WIDTH + col] =
                (tile & 0xFC00) | (tileIndex & 0x03FF);
        }

        /* Keep the two unused columns clear. */
        dst[row * INTRO_BG_SCREEN_WIDTH + 30] = 0;
        dst[row * INTRO_BG_SCREEN_WIDTH + 31] = 0;
    }

    /* Clear the twelve unused rows in the 32x32 screen block. */
    for (row = INTRO_BG_TILEMAP_HEIGHT; row < INTRO_BG_SCREEN_HEIGHT; row++)
    {
        for (col = 0; col < INTRO_BG_SCREEN_WIDTH; col++)
            dst[row * INTRO_BG_SCREEN_WIDTH + col] = 0;
    }
}

#define TEST2_LOOP_BG_WIDTH 32
#define TEST2_LOOP_BG_HEIGHT 32

static void CopyTest2LoopBgTilemap(void)
{
    const u16 *src = sTest2LoopBgTilemap;
    u16 *dst = (u16 *)BG_SCREEN_ADDR(TEST2_LOOP_BG_SCREENBASE);
    u16 row;
    u16 col;

    for (row = 0; row < TEST2_LOOP_BG_HEIGHT; row++)
    {
        for (col = 0; col < TEST2_LOOP_BG_WIDTH; col++)
        {
            u16 tile = src[row * TEST2_LOOP_BG_WIDTH + col];
            u16 tileIndex = (tile & 0x03FF) + TEST2_LOOP_TILE_OFFSET;

            dst[row * TEST2_LOOP_BG_WIDTH + col] =
                (tile & 0x0C00)
                | (tileIndex & 0x03FF)
                | (TEST2_LOOP_BG_PALETTE << 12);
        }
    }
}

static void VBlankCB_Intro(void)
{
LoadOam();
ProcessSpriteCopyRequests();

// The tilemap is small enough to update during VBlank. This prevents
 // the display from seeing a partially replaced map.
 if (sIntroTilemapPending)
 {
 CopyTest2IntroTilemap(sIntroPendingTilemap);
 sIntroTilemapPending = FALSE;
 }

TransferPlttBuffer();

}

void MainCB2_Intro(void)
{
RunTasks();
AnimateSprites();
BuildOamBuffer();
UpdatePaletteFade();
if (JOY_NEW(A_BUTTON | B_BUTTON | START_BUTTON | SELECT_BUTTON) && !gPaletteFade.active)
SetMainCallback2(MainCB2_EndIntro);
else if (gIntroFrameCounter != -1)
gIntroFrameCounter++;
}

static void MainCB2_EndIntro(void)
{
    /*
     * Wait until any active fade has finished before leaving the intro.
     */
    if (UpdatePaletteFade())
        return;

    /*
     * Stop every intro-specific display effect before title-screen
     * initialization begins.
     */
    SetHBlankCallback(NULL);
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();
    SetVBlankCallback(NULL);

    /*
     * Hide the intro completely before handing control to the title
     * screen. This prevents one stale intro frame from being displayed.
     */
    SetGpuReg(REG_OFFSET_DISPCNT, 0);

    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);

    SetGpuReg(REG_OFFSET_BG2PA, 0x100);
    SetGpuReg(REG_OFFSET_BG2PB, 0);
    SetGpuReg(REG_OFFSET_BG2PC, 0);
    SetGpuReg(REG_OFFSET_BG2PD, 0x100);
    SetGpuReg(REG_OFFSET_BG2X_L, 0);
    SetGpuReg(REG_OFFSET_BG2X_H, 0);
    SetGpuReg(REG_OFFSET_BG2Y_L, 0);
    SetGpuReg(REG_OFFSET_BG2Y_H, 0);

    /*
     * The title screen will perform its own graphics initialization
     * while the display is hidden.
     */
    gMain.state = 0;
    SetMainCallback2(CB2_InitTitleScreen);
}

static void LoadCopyrightGraphics(u16 tilesetAddress, u16 tilemapAddress, u16 paletteOffset)
{
    DecompressDataWithHeaderVram(gIntroCopyright_Gfx, (void *)(VRAM + tilesetAddress));
    DecompressDataWithHeaderVram(gIntroCopyright_Tilemap, (void *)(VRAM + tilemapAddress));
    LoadPalette(gIntroCopyright_Pal, paletteOffset, PLTT_SIZE_4BPP);
}

static void SerialCB_CopyrightScreen(void)
{
GameCubeMultiBoot_HandleSerialInterrupt(&gMultibootProgramStruct);
}

static u8 SetUpCopyrightScreen(void)
{
if (IS_FRLG)
return SetUpCopyrightScreenFrlg();

switch (gMain.state)
 {
 case COPYRIGHT_INITIALIZE:
 SetVBlankCallback(NULL);
 SetGpuReg(REG_OFFSET_BLDCNT, 0);
 SetGpuReg(REG_OFFSET_BLDALPHA, 0);
 SetGpuReg(REG_OFFSET_BLDY, 0);
 *(u16 *)PLTT = RGB_WHITE;
 SetGpuReg(REG_OFFSET_DISPCNT, 0);
 SetGpuReg(REG_OFFSET_BG0HOFS, 0);
 SetGpuReg(REG_OFFSET_BG0VOFS, 0);
 CpuFill32(0, (void *)VRAM, VRAM_SIZE);
 CpuFill32(0, (void *)OAM, OAM_SIZE);
 CpuFill16(0, (void *)(PLTT + 2), PLTT_SIZE - 2);
 ResetPaletteFade();
 LoadCopyrightGraphics(0, 0x3800, BG_PLTT_ID(0));
 ScanlineEffect_Stop();
 ResetTasks();
 ResetSpriteData();
 FreeAllSpritePalettes();
 BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
 SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0)
 | BGCNT_CHARBASE(0)
 | BGCNT_SCREENBASE(7)
 | BGCNT_16COLOR
 | BGCNT_TXT256x256);
 EnableInterrupts(INTR_FLAG_VBLANK);
 SetVBlankCallback(VBlankCB_Intro);
 REG_DISPCNT = DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON;
 SetSerialCallback(SerialCB_CopyrightScreen);
 GameCubeMultiBoot_Init(&gMultibootProgramStruct);
 case COPYRIGHT_EMULATOR_BLEND:
 REG_DISPCNT = DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG0_ON;
 default:
 UpdatePaletteFade();
 gMain.state++;
 GameCubeMultiBoot_Main(&gMultibootProgramStruct);
 break;
 case COPYRIGHT_START_FADE:
 GameCubeMultiBoot_Main(&gMultibootProgramStruct);
 if (gMultibootProgramStruct.gcmb_field_2 != 1)
 {
 BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_BLACK);
 gMain.state++;
 }
 break;
 case COPYRIGHT_START_INTRO:
 if (UpdatePaletteFade())
 break;

#if EXPANSION_INTRO == TRUE
SetMainCallback2(CB2_ExpansionIntro);
CreateTask(Task_HandleExpansionIntro, 0);
#else
CreateTask(Task_Scene1_Load, 0);
SetMainCallback2(MainCB2_Intro);
#endif
if (gMultibootProgramStruct.gcmb_field_2 != 0)
{
if (gMultibootProgramStruct.gcmb_field_2 == 2)
{
if (*(u32 *)(EWRAM_START + 0xAC) == COLOSSEUM_GAME_CODE)
{
CpuCopy16(&gMultiBootProgram_PokemonColosseum_Start, (void *)EWRAM_START, sizeof(gMultiBootProgram_PokemonColosseum_Start));
*(u32 *)(EWRAM_START + 0xAC) = COLOSSEUM_GAME_CODE;
}
GameCubeMultiBoot_ExecuteProgram(&gMultibootProgramStruct);
}
}
else
{
GameCubeMultiBoot_Quit();
SetSerialCallback(SerialCB);
}
return 0;
}

return 1;

}

void CB2_InitCopyrightScreenAfterBootup(void)
{
if (!SetUpCopyrightScreen())
{
SetSaveBlocksPointers(GetSaveBlocksPointersBaseOffset());
ResetMenuAndMonGlobals();
Save_ResetSaveCounters();
LoadGameSave(SAVE_NORMAL);
if (gSaveFileStatus == SAVE_STATUS_EMPTY || gSaveFileStatus == SAVE_STATUS_CORRUPT)
Sav2_ClearSetDefault();
SetPokemonCryStereo(gSaveBlock2Ptr->optionsSound);
DynPal_InitAllDynamicPalettes();
InitHeap(gHeap, HEAP_SIZE);
}
}

void CB2_InitCopyrightScreenAfterTitleScreen(void)
{
SetUpCopyrightScreen();
}

#define sBigDropSpriteId data[0]

static void Task_IntroBackgroundAnimation(u8 taskId)
{
    u8 background;

    sTest2LoopBgScroll += TEST2_BG_SCROLL_SPEED;
    SetGpuReg(REG_OFFSET_BG0HOFS, sTest2LoopBgScroll);

    if (gTasks[taskId].tIntroBgTimer != 0)
    {
        gTasks[taskId].tIntroBgTimer--;
        return;
    }

    gTasks[taskId].tIntroBgFrame++;

    if (gTasks[taskId].tIntroBgFrame >= ARRAY_COUNT(sIntroBgAnimation))
        gTasks[taskId].tIntroBgFrame = 0;

    background = sIntroBgAnimation[gTasks[taskId].tIntroBgFrame].background;
    sIntroPendingTilemap = background;
    sIntroTilemapPending = TRUE;

    gTasks[taskId].tIntroBgTimer =
        sIntroBgAnimation[gTasks[taskId].tIntroBgFrame].duration - 1;
}

static void CopyTest2IntroTilemapFromSource(const u16 *src, u16 tileOffset)
{
    u16 *dst = (u16 *)BG_SCREEN_ADDR(TEST2_SCENE_SCREENBASE);
    u8 row;
    u8 col;

    /* Elemental maps are full 32x32 GBA text maps (2KB each). */
    for (row = 0; row < 32; row++)
    {
        for (col = 0; col < 32; col++)
        {
            u16 tile = src[row * 32 + col];
            u16 tileIndex = (tile & 0x03FF) + tileOffset;

            dst[row * 32 + col] =
                (tile & 0xFC00) | (tileIndex & 0x03FF);
        }
    }
}

static void Task_Test2_LoadMonBackground(u8 taskId)
{
    const struct Test2MonData *mon;
    u8 monNumber;

    monNumber = gTasks[taskId].tTest2MonNumber;
    mon = &sTest2MonData[monNumber];

    Task_Test2_HideRunningSprites(taskId);

    /*
     * All elemental graphics were loaded once by Task_Test2_LoadGraphics.
     * Only replace the 32x32 map and its 16-bank 4bpp palette here.
     * BG0 and BG2 are disabled while the elemental scene is active.
     */
    sIntroTilemapPending = FALSE;
    CopyTest2IntroTilemapFromSource(
        mon->backgroundTilemap,
        mon->backgroundTileOffset
    );

    LoadPalette(
        mon->backgroundPal,
        BG_PLTT_ID(0),
        PLTT_SIZE_4BPP * 16
    );
    TransferPlttBuffer();

    SetGpuReg(
        REG_OFFSET_BG1CNT,
        BGCNT_PRIORITY(0)
        | BGCNT_CHARBASE(mon->backgroundCharbase)
        | BGCNT_SCREENBASE(TEST2_SCENE_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );

    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);

    SetGpuReg(
        REG_OFFSET_DISPCNT,
        DISPCNT_MODE_0
        | DISPCNT_OBJ_1D_MAP
        | DISPCNT_BG1_ON
        | DISPCNT_OBJ_ON
    );

    gTasks[taskId].tTest2MonState = TEST2_MON_STATE_RELEASE;
    gTasks[taskId].tTest2MonTimer = 0;
    gTasks[taskId].tTest2MonCryPlayed = FALSE;
    gTasks[taskId].func = Task_Test2_MonRelease;
}

static void Task_Test2_MonRelease(u8 taskId)
{
    u16 species;
    u8 spriteId;
    struct Sprite *sprite;

    if (gTasks[taskId].tTest2MonSpriteId == SPRITE_NONE)
    {
        species = sTest2MonData[gTasks[taskId].tTest2MonNumber].species;

        spriteId = CreateMonPicSprite_Affine(
            species,
            FALSE,
            0,
            MON_PIC_AFFINE_FRONT,
            TEST2_MON_X,
            TEST2_MON_Y,
            14,
            TAG_NONE
        );

        gTasks[taskId].tTest2MonSpriteId = spriteId;

        sprite = &gSprites[spriteId];
        sprite->callback = SpriteCallbackDummy;
        sprite->invisible = FALSE;
        sprite->oam.priority = 0;

        CreatePokeballSpriteToReleaseMon(
            spriteId,
            sprite->oam.paletteNum,
            TEST2_MON_X,
            TEST2_MON_Y,
            0,
            0,
            32,
            PALETTES_BG,
            species
        );

        return;
    }

    sprite = &gSprites[gTasks[taskId].tTest2MonSpriteId];

    if (sprite->callback != SpriteCallbackDummy)
    {
        return;
    }

    sprite->oam.affineMode = ST_OAM_AFFINE_OFF;
    gTasks[taskId].tTest2MonTimer = TEST2_MON_HOLD_FRAMES;
    gTasks[taskId].tTest2MonState = TEST2_MON_STATE_HOLD;
    gTasks[taskId].func = Task_Test2_MonHold;
}

static void Task_Test2_MonHold(u8 taskId)
{
    if (gTasks[taskId].tTest2MonTimer != 0)
    {
        gTasks[taskId].tTest2MonTimer--;

        if (!gTasks[taskId].tTest2MonCryPlayed)
        {
            Task_Test2_PlayMonCry(taskId);
        }

        return;
    }

    gTasks[taskId].tTest2MonState = TEST2_MON_STATE_RETURN;
    gTasks[taskId].func = Task_Test2_MonReturn;
}

static void Task_Test2_PlayMonCry(u8 taskId)
{
    u16 species;

    species = sTest2MonData[gTasks[taskId].tTest2MonNumber].species;

    if (IsSpeciesEnabled(species))
    {
        PlayCryInternal(
            species,
            0,
            100,
            CRY_PRIORITY_NORMAL,
            CRY_MODE_NORMAL
        );
    }

    gTasks[taskId].tTest2MonCryPlayed = TRUE;
}

static void Task_Test2_MonReturn(u8 taskId)
{
    u8 spriteId;

    spriteId = gTasks[taskId].tTest2MonSpriteId;

    if (spriteId != SPRITE_NONE)
    {
        FreeAndDestroyMonPicSprite(spriteId);
        gTasks[taskId].tTest2MonSpriteId = SPRITE_NONE;
    }

    Task_Test2_RestoreRunningScene(taskId);

    gTasks[taskId].tTest2MonNumber++;

    if (gTasks[taskId].tTest2MonNumber > TEST2_MON_FEVINE)
    {
        gTasks[taskId].tTest2MonNumber = TEST2_MON_FEVINE + 1;
    }

    gTasks[taskId].tTest2MonState = TEST2_MON_STATE_NONE;
    gTasks[taskId].func = Task_Test2_Opening;
}

static void Task_Test2_RestoreRunningScene(u8 taskId)
{
    Task_Test2_ShowRunningSprites(taskId);

    /*
     * No character graphics are recopied: the loop, runner, and all three
     * elemental tilesets remain resident in BG VRAM for the whole scene.
     * Restore only the two 16-color palette banks used by the runner scene
     * and the runner's animated map in the shared screen block.
     */
    LoadPalette(
        sTestBgSharedPal,
        BG_PLTT_ID(0),
        sizeof(sTestBgSharedPal)
    );

    LoadPalette(
        sTest2LoopBgPal,
        BG_PLTT_ID(TEST2_LOOP_BG_PALETTE),
        PLTT_SIZE_4BPP
    );

    sIntroTilemapPending = FALSE;
    CopyTest2IntroTilemap(0);

    /* Restore the window and black backdrop as well as the BG layers. */
    Test2SetCinematicBars(sTest2BarHeight);

    SetGpuReg(REG_OFFSET_BG0HOFS, sTest2LoopBgScroll);
    Test2ApplySharedDisplacement(gTasks[taskId].tTest2Displacement);

    TransferPlttBuffer();
}

void Task_Scene1_Load(u8 taskId)
{
    /*
     * Hide the copyright scene and stop its display effects.
     * Do not clear or replace graphics during this first stage.
     */
    PrepareIntroSceneTransition();

    gTasks[taskId].func = Task_Scene1_LoadPrepare;
}

static void Task_Scene1_LoadPrepare(u8 taskId)
{
    /*
     * This runs on the next task frame, after the display has been
     * disabled. Clear the old scene before loading Test 1.
     */
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();

    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)(PLTT + 2), PLTT_SIZE - 2);

    gTasks[taskId].func = Task_Scene1_LoadGraphics;
}

static void Task_Scene1_LoadGraphics(u8 taskId)
{
    u16 skyColor;
    u8 spriteId;

    /*
     * Load the Test 1 affine water background.
     */
    CpuCopy16(
        sTest_1WaterGfx,
        (void *)BG_CHAR_ADDR(0),
        sizeof(sTest_1WaterGfx)
    );

    CopyTest_1WaterMap();

    LoadPalette(
        sTest_1WaterPal,
        BG_PLTT_ID(0),
        sizeof(sTest_1WaterPal)
    );

    skyColor = RGB(27, 28, 30);
    LoadPalette(
        &skyColor,
        BG_PLTT_ID(0),
        sizeof(skyColor)
    );

    /*
     * Load the Test 1 mountain background.
     */
    CpuCopy16(
        sTest_1MountainGfx,
        (void *)BG_CHAR_ADDR(TEST1_MOUNTAIN_CHARBASE),
        sizeof(sTest_1MountainGfx)
    );

    CopyTest_1MountainMap();

    LoadPalette(
        sTest_1MountainPal,
        BG_PLTT_ID(TEST1_MOUNTAIN_PALETTE),
        sizeof(sTest_1MountainPal)
    );

    /*
     * Load the Game Freak logo and its sprites.
     */
    LoadCompressedSpriteSheet(sSpriteSheet_WaterDropsAndLogo);
    LoadSpritePalettes(sSpritePalettes_Intro1);
    CreateGameFreakLogoSprites(120, 88, 0);

    /*
     * Load the DWorks logo.
     */
    LoadCompressedSpriteSheet(&sSpriteSheet_DWorksLogo);
    LoadSpritePalette(&sSpritePalette_DWorksLogo);

    spriteId = CreateSprite(
        &sSpriteTemplate_DWorksLogo,
        120,
        88,
        0
    );

    gSprites[spriteId].invisible = TRUE;

    /*
     * Configure Test 1 backgrounds while the display is disabled.
     */
    SetGpuReg(
        REG_OFFSET_BG1CNT,
        BGCNT_PRIORITY(2)
        | BGCNT_CHARBASE(TEST1_MOUNTAIN_CHARBASE)
        | BGCNT_SCREENBASE(TEST1_MOUNTAIN_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );

    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(
        REG_OFFSET_BG1VOFS,
        TEST1_MOUNTAIN_INITIAL_VOFS
    );

    SetGpuReg(
        REG_OFFSET_BG2CNT,
        BGCNT_PRIORITY(1)
        | BGCNT_CHARBASE(0)
        | BGCNT_SCREENBASE(30)
        | BGCNT_256COLOR
        | BGCNT_WRAP
        | BGCNT_AFF256x256
    );

    PanAndScaleWater(
        DISPLAY_WIDTH / 2,
        0,
        256,
        256,
        0
    );

    ScanlineEffect_Stop();
    ScanlineEffect_Clear();

    /*
     * Install Test 1's HBlank callback only after the old one has
     * been stopped and the new graphics are ready.
     */
    SetHBlankCallback(IntroWaterHBlankCallback);

    BeginNormalPaletteFade(
        PALETTES_ALL,
        0,
        16,
        0,
        RGB_BLACK
    );

    /*
     * Install VBlank last, while the display is still hidden.
     */
    SetVBlankCallback(VBlankCB_Intro);
    EnableInterrupts(
        INTR_FLAG_VBLANK
        | INTR_FLAG_HBLANK
    );

    sIntroTilemapPending = FALSE;

    gTasks[taskId].tTest1WaterFrame = 0;
    gTasks[taskId].tTest1WaterScroll = 0;
    gTasks[taskId].tTest1MountainState =
        TEST1_MOUNTAIN_MOVING;
    gTasks[taskId].tTest1MountainHold = 0;

    sIntroWaterScroll = 0;
    gIntroFrameCounter = 0;

    ResetSerial();

    /*
     * Task_Test1_Show runs on the following task frame and enables
     * the display after the new callbacks have been installed.
     */
    gTasks[taskId].func = Task_Test1_Show;
}

static void Task_Test1_Show(u8 taskId)
{
    SetGpuReg(
        REG_OFFSET_DISPCNT,
        DISPCNT_MODE_1
        | DISPCNT_OBJ_1D_MAP
        | DISPCNT_BG2_ON
        | DISPCNT_OBJ_ON
    );

    gTasks[taskId].func = Task_Test1_Water;
}

#undef tIntroBgFrame
#undef tIntroBgTimer

static u16 GetTest1Horizon(void)
{
u32 elapsed;

if (gIntroFrameCounter <= TEST1_CAMERA_PAN_END)
 return 0;
 elapsed = gIntroFrameCounter - TEST1_CAMERA_PAN_END;
 if (elapsed >= TEST1_PERSPECTIVE_FRAMES)
 return TEST1_HORIZON_Y;
 // Smoothstep easing avoids a sudden acceleration at the start/end.
 elapsed = (elapsed * 256) / TEST1_PERSPECTIVE_FRAMES;
 elapsed = (elapsed * elapsed * (768 - 2 * elapsed)) / (256 * 256);
 return (TEST1_HORIZON_Y * elapsed) / 256;

}

/*

• GBA affine ground-plane projection, adapted from the per-scanline technique

• used by the Soar/Mode-7 style example. BG2PA and BG2PC describe the horizontal

• texture step for this scanline; BG2X/BG2Y anchor the scanline at the camera.

• The sky is the backdrop color (palette entry 0), revealed by disabling BG2

• above the animated horizon.
*/
static void IntroWaterHBlankCallback(void)
{
s32 line = ((s32)REG_VCOUNT + 1) % 228;
s32 horizon = GetTest1Horizon();
s32 elapsed;
s32 blend;
s32 distance;
s32 targetScale;
s32 scale;
s32 flatX = 0x8000 - (120 * 256);
s32 flatY = (s32)sIntroWaterScroll << 8;
s32 targetX;
s32 targetY;
s32 cameraX = 0x8000;
s32 cameraY = flatY;

if (line < 0 || line >= DISPLAY_HEIGHT)
return;

// Smoothly blend from a completely flat affine background to the
// receding-plane projection. This keeps the opening frames undistorted.
if (gIntroFrameCounter <= TEST1_CAMERA_PAN_END)
blend = 0;
else
{
elapsed = gIntroFrameCounter - TEST1_CAMERA_PAN_END;
if (elapsed >= TEST1_PERSPECTIVE_FRAMES)
blend = 256;
else
{
elapsed = (elapsed * 256) / TEST1_PERSPECTIVE_FRAMES;
blend = (elapsed * elapsed * (768 - 2 * elapsed)) / (256 * 256);
}
}

if (line < horizon)
{
REG_DISPCNT &= ~DISPCNT_BG2_ON;
return;
}
REG_DISPCNT |= DISPCNT_BG2_ON;

distance = line - horizon + 1;
targetScale = (TEST1_CAMERA_HEIGHT * 256) / distance;
if (targetScale > 0x7FFF)
targetScale = 0x7FFF;
if (targetScale < 32)
targetScale = 32;

scale = 256 + ((targetScale - 256) * blend) / 256;
targetX = cameraX - (120 * targetScale);
targetY = cameraY - ((line - horizon) * 0x100);

REG_BG2PA = (u16)scale;
REG_BG2PB = 0;
REG_BG2PC = 0;
REG_BG2PD = 0x100;
REG_BG2X = flatX + ((targetX - flatX) * blend) / 256;
REG_BG2Y = flatY + ((targetY - flatY) * blend) / 256;
}

static void Task_Test1_Transition(u8 taskId)
{
    // Do not start scene animation until the fade-in has fully completed.
    if (!gPaletteFade.active)
        gTasks[taskId].func = Task_Test1_Water;
}

static void Task_Test1_Water(u8 taskId)
{
u16 horizon;

// Keep the water moving independently of the camera movement.
 gTasks[taskId].tTest1WaterScroll += TEST1_WATER_SCROLL_SPEED;
 sIntroWaterScroll = gTasks[taskId].tTest1WaterScroll;

// Reset the affine reference each frame. The HBlank callback then
 // replaces the affine values for each visible scanline.
 // PanAndScaleWater(DISPLAY_WIDTH / 2, 0, 256, 256, 0);

horizon = GetTest1Horizon();

/*
• Before the perspective phase, BG1 remains disabled.
• During the perspective phase, use the exact same horizon
• displacement as the water projection. For a text background,
• subtracting from VOFS moves the artwork downward.
 */
 if (gIntroFrameCounter > TEST1_CAMERA_PAN_END)
 {
 REG_DISPCNT |= DISPCNT_BG1_ON;

SetGpuReg(REG_OFFSET_BG1VOFS,
 TEST1_MOUNTAIN_INITIAL_VOFS - horizon);
 }

if (gIntroFrameCounter >= TEST1_CAMERA_PAN_END + TEST1_PERSPECTIVE_FRAMES)
 {
 if (gTasks[taskId].tTest1MountainState == TEST1_MOUNTAIN_MOVING)
 {
 gTasks[taskId].tTest1MountainState = TEST1_MOUNTAIN_HOLDING;
 gTasks[taskId].tTest1MountainHold = TEST1_MOUNTAIN_HOLD_FRAMES;
 }
 else if (gTasks[taskId].tTest1MountainState == TEST1_MOUNTAIN_HOLDING)
 {
 if (gTasks[taskId].tTest1MountainHold != 0)
 gTasks[taskId].tTest1MountainHold--;

if (gTasks[taskId].tTest1MountainHold == 0)
 {
 BeginNormalPaletteFade(PALETTES_ALL,
 0,
 0,
 TEST1_MOUNTAIN_FADE_FRAMES,
 RGB_WHITEALPHA);

gTasks[taskId].tTest1MountainState = TEST1_MOUNTAIN_FADING;
 }
 }
 else if (gTasks[taskId].tTest1MountainState == TEST1_MOUNTAIN_FADING)
 {
 if (!gPaletteFade.active)
 gTasks[taskId].func = Task_Test2_Load;
 }
 }

}

static void Test2ApplySharedDisplacement(s16 displacement)
{
    sTest2SharedDisplacement = displacement;

    /*
     * Both backgrounds use the same vertical displacement.
     * BG0's horizontal movement remains independent.
     */
    SetGpuReg(REG_OFFSET_BG0VOFS, displacement);
    SetGpuReg(REG_OFFSET_BG2VOFS, displacement);
}

static void Test2SetCinematicBars(u8 barHeight)
{
    u16 black = RGB_BLACK;

    sTest2BarHeight = barHeight;

    if (barHeight == 0)
    {
        SetGpuReg(
            REG_OFFSET_DISPCNT,
            DISPCNT_MODE_0
            | DISPCNT_OBJ_1D_MAP
            | DISPCNT_BG0_ON
            | DISPCNT_BG2_ON
            | DISPCNT_OBJ_ON
        );
        return;
    }

    /* Palette entry 0 is the backdrop color outside WIN0. Elemental
     * palettes replace the BG palette during the mon scene, so restore
     * black whenever the cinematic window is enabled again. */
    LoadPalette(&black, 0, sizeof(black));

    /* WIN0 contains the visible part of the scene. */
    SetGpuReg(REG_OFFSET_WIN0H, WIN_RANGE(0, DISPLAY_WIDTH));
    SetGpuReg(
        REG_OFFSET_WIN0V,
        WIN_RANGE(barHeight, DISPLAY_HEIGHT - barHeight)
    );

    /*
     * Show all backgrounds and objects inside the window.
     * Show only the backdrop outside the window.
     */
    SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL);
    SetGpuReg(REG_OFFSET_WINOUT, 0);

    SetGpuReg(
        REG_OFFSET_DISPCNT,
        DISPCNT_MODE_0
        | DISPCNT_OBJ_1D_MAP
        | DISPCNT_BG0_ON
        | DISPCNT_BG2_ON
        | DISPCNT_OBJ_ON
        | DISPCNT_WIN0_ON
    );
}

static void Test2EnableCinematicBars(void)
{
    Test2SetCinematicBars(TEST2_CINEMATIC_BAR_HEIGHT);
}

static void Test2UpdateOpeningPan(struct Task *task)
{
    u32 elapsed = task->tTest2Timer;

    if (elapsed >= TEST2_PAN_FRAMES)
    {
        Test2ApplySharedDisplacement(TEST2_END_VOFS);
        return;
    }

    /*
     * Start at TEST2_START_VOFS (208) and move to TEST2_END_VOFS (-48) smoothly over time.
     */
    s32 currentVofs = TEST2_START_VOFS - (s32)((TEST2_TOTAL_VOFS_DISTANCE * elapsed) / TEST2_PAN_FRAMES);

    task->tTest2Displacement = currentVofs;
    Test2ApplySharedDisplacement(currentVofs);
}

static void Test2UpdateBarRetraction(struct Task *task)
{
    u8 elapsed = task->tTest2Timer;

    if (elapsed >= TEST2_BAR_RETRACT_FRAMES)
    {
        Test2SetCinematicBars(0);
        Test2ApplySharedDisplacement(0);
        return;
    }

    // Retract bars smoothly while normalizing offset
    u8 currentBarHeight = TEST2_CINEMATIC_BAR_HEIGHT - ((TEST2_CINEMATIC_BAR_HEIGHT * elapsed) / TEST2_BAR_RETRACT_FRAMES);
    Test2SetCinematicBars(currentBarHeight);

    s32 currentVofs = TEST2_END_VOFS - ((TEST2_END_VOFS * (s32)elapsed) / TEST2_BAR_RETRACT_FRAMES);
    Test2ApplySharedDisplacement(currentVofs);
}

static void Task_Test2_CreateRunningSprites(u8 taskId)
{
    u8 spriteId;

    LoadCompressedSpriteSheet(&sSpriteSheet_RunningPokemon[0]);
    LoadCompressedSpriteSheet(&sSpriteSheet_RunningPokemon[1]);
    LoadCompressedSpriteSheet(&sSpriteSheet_RunningPokemon[2]);
    LoadSpritePalettes(sSpritePalettes_RunningPokemon);

    spriteId = CreateSprite(
        &sSpriteTemplate_Volbeat,
        DISPLAY_WIDTH + 32,
        80,
        4
    );
    gSprites[spriteId].callback = SpriteCallbackDummy;
    gSprites[spriteId].invisible = FALSE;
    gTasks[taskId].tTest2VolbeatSpriteId = spriteId;

    spriteId = CreateSprite(
        &sSpriteTemplate_Torchic,
        DISPLAY_WIDTH + 48,
        110,
        1
    );
    gSprites[spriteId].callback = SpriteCallbackDummy;
    gSprites[spriteId].invisible = FALSE;
    gTasks[taskId].tTest2TorchicSpriteId = spriteId;

    spriteId = CreateSprite(
        &sSpriteTemplate_Manectric,
        DISPLAY_WIDTH + 32,
        128,
        0
    );
    gSprites[spriteId].callback = SpriteCallbackDummy;
    gSprites[spriteId].invisible = FALSE;
    gTasks[taskId].tTest2ManectricSpriteId = spriteId;
}

static void Task_Test2_HideRunningSprites(u8 taskId)
{
    if (gTasks[taskId].tTest2VolbeatSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2VolbeatSpriteId].invisible = TRUE;
    }

    if (gTasks[taskId].tTest2TorchicSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2TorchicSpriteId].invisible = TRUE;
    }

    if (gTasks[taskId].tTest2ManectricSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2ManectricSpriteId].invisible = TRUE;
    }
}

static void Task_Test2_ShowRunningSprites(u8 taskId)
{
    if (gTasks[taskId].tTest2VolbeatSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2VolbeatSpriteId].invisible = FALSE;
    }

    if (gTasks[taskId].tTest2TorchicSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2TorchicSpriteId].invisible = FALSE;
    }

    if (gTasks[taskId].tTest2ManectricSpriteId != SPRITE_NONE)
    {
        gSprites[gTasks[taskId].tTest2ManectricSpriteId].invisible = FALSE;
    }
}

static void Task_Test2_Opening(u8 taskId)
{
    struct Task *task;
    u32 monInterval;

    task = &gTasks[taskId];
    monInterval = TEST2_MON_INTERVAL_FRAMES;

    if (task->tTest2State == TEST2_STATE_PAN)
    {
        Task_IntroBackgroundAnimation(taskId);
        Test2UpdateOpeningPan(task);

        /*
         * Every quarter of the full pan, interrupt the runners and
         * display the next Pokémon.
         */
        if (task->tTest2MonState == TEST2_MON_STATE_NONE
            && task->tTest2MonNumber <= TEST2_MON_FEVINE
            && task->tTest2Timer >= task->tTest2NextMonFrame)
        {
            task->tTest2NextMonFrame += TEST2_MON_INTERVAL_FRAMES;
            task->tTest2MonState = TEST2_MON_STATE_LOAD_BACKGROUND;
            task->func = Task_Test2_LoadMonBackground;
            return;
        }
    }
    else
    {
        Task_IntroBackgroundAnimation(taskId);
    }

    switch (task->tTest2State)
    {
    case TEST2_STATE_PAN:
        if (task->tTest2Timer++ >= TEST2_PAN_FRAMES)
        {
            task->tTest2State = TEST2_STATE_TOP_HOLD;
            task->tTest2Timer = 0;
            Test2ApplySharedDisplacement(TEST2_END_VOFS);
        }
        break;

    case TEST2_STATE_TOP_HOLD:
        if (task->tTest2Timer++ >= TEST2_TOP_HOLD_FRAMES)
        {
            task->tTest2State = TEST2_STATE_RETRACT_BARS;
            task->tTest2Timer = 0;
        }
        break;

    case TEST2_STATE_RETRACT_BARS:
        Test2UpdateBarRetraction(task);

        if (task->tTest2Timer++ >= TEST2_BAR_RETRACT_FRAMES)
        {
            task->tTest2State = TEST2_STATE_OPEN_RUN;
            task->tTest2Timer = 0;

            Test2SetCinematicBars(0);
            Test2ApplySharedDisplacement(0);
        }
        break;

    case TEST2_STATE_OPEN_RUN:
        if (task->tTest2Timer++ >= TEST2_OPEN_RUN_FRAMES)
        {
            task->tTest2State = TEST2_STATE_FADE_TO_BLACK;
            task->tTest2Timer = 0;

            // Give the final fade a little more weight before Test 3.
            BeginNormalPaletteFade(
                PALETTES_ALL,
                2,
                0,
                16,
                RGB_BLACK
            );
        }
        break;

    case TEST2_STATE_FADE_TO_BLACK:
        if (!gPaletteFade.active)
        {
            task->tTest2State = TEST2_STATE_WAIT_FOR_FADE;
        }
        break;

    case TEST2_STATE_WAIT_FOR_FADE:
        SetGpuReg(REG_OFFSET_DISPCNT, 0);
        task->tTest2Timer = 0;
        task->func = Task_Test3_LoadMountains;
        break;
    }
}

static void Task_Test2_Load(u8 taskId)
{
    /*
     * Hide Test 1 and stop its HBlank/VBlank display activity.
     * The graphics are not cleared during this first stage.
     */
    PrepareIntroSceneTransition();

    gTasks[taskId].func = Task_Test2_LoadPrepare;
}

static void Task_Test2_LoadPrepare(u8 taskId)
{
    /*
     * This runs on the next task frame while DISPCNT is disabled.
     */
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();

    DmaFill16(
        3,
        0,
        (void *)VRAM,
        VRAM_SIZE
    );

    DmaFill32(
        3,
        0,
        (void *)OAM,
        OAM_SIZE
    );

    DmaFill16(
        3,
        0,
        (void *)(PLTT + 2),
        PLTT_SIZE - 2
    );

    gTasks[taskId].func = Task_Test2_LoadGraphics;
}

static void Task_Test2_LoadGraphics(u8 taskId)
{
    /*
     * Load every Test 2 tileset once while the display is hidden.
     *
     * BG VRAM layout (all graphics are 4bpp, 32 bytes per tile):
     *   0x0000: fire tiles
     *   after fire: runner/test tiles
     *   0x8000: water tiles
     *   after water: grass tiles
     *   after grass: loop tiles
     *
     * The runner tiles occupy the remainder of the first 32KB character
     * window. Water, grass, and loop occupy the second 32KB window. The
     * loop map is in screen block 28; screen block 30 is reused for either
     * the runner map or the currently displayed elemental map.
     */
    CpuCopy16(
        sTest2FireBgGfx,
        (void *)BG_CHAR_ADDR(0),
        sizeof(sTest2FireBgGfx)
    );

    CpuCopy16(
        sTestBgSharedGfx,
        (void *)((u8 *)BG_CHAR_ADDR(0) + sizeof(sTest2FireBgGfx)),
        sizeof(sTestBgSharedGfx)
    );

    CpuCopy16(
        sTest2WaterBgGfx,
        (void *)BG_CHAR_ADDR(2),
        sizeof(sTest2WaterBgGfx)
    );

    CpuCopy16(
        sTest2GrassBgGfx,
        (void *)((u8 *)BG_CHAR_ADDR(2) + sizeof(sTest2WaterBgGfx)),
        sizeof(sTest2GrassBgGfx)
    );

    CpuCopy16(
        sTest2LoopBgGfx,
        (void *)((u8 *)BG_CHAR_ADDR(2)
            + sizeof(sTest2WaterBgGfx)
            + sizeof(sTest2GrassBgGfx)),
        sizeof(sTest2LoopBgGfx)
    );

    LoadPalette(
        sTestBgSharedPal,
        BG_PLTT_ID(0),
        sizeof(sTestBgSharedPal)
    );

    /* The loop map is forced to palette bank 1, so only that bank is needed. */
    LoadPalette(
        sTest2LoopBgPal,
        BG_PLTT_ID(TEST2_LOOP_BG_PALETTE),
        PLTT_SIZE_4BPP
    );

    sIntroTilemapPending = FALSE;
    CopyTest2IntroTilemap(0);
    CopyTest2LoopBgTilemap();

    sTest2LoopBgScroll = 0;

    SetGpuReg(
        REG_OFFSET_BG0CNT,
        BGCNT_PRIORITY(1)
        | BGCNT_CHARBASE(TEST2_LOOP_BG_CHARBASE)
        | BGCNT_SCREENBASE(TEST2_LOOP_BG_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
        | BGCNT_WRAP
    );

    SetGpuReg(
        REG_OFFSET_BG1CNT,
        BGCNT_PRIORITY(0)
        | BGCNT_CHARBASE(0)
        | BGCNT_SCREENBASE(TEST2_SCENE_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );

    SetGpuReg(
        REG_OFFSET_BG2CNT,
        BGCNT_PRIORITY(0)
        | BGCNT_CHARBASE(0)
        | BGCNT_SCREENBASE(TEST2_SCENE_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );

    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);

    Test2ApplySharedDisplacement(TEST2_START_VOFS);

    Task_Test2_CreateRunningSprites(taskId);

    SetVBlankCallback(VBlankCB_Intro);
    EnableInterrupts(INTR_FLAG_VBLANK);

    TransferPlttBuffer();

    gTasks[taskId].tTest2State = TEST2_STATE_PAN;
    gTasks[taskId].tTest2Timer = 0;
    gTasks[taskId].tTest2Displacement = TEST2_START_VOFS;
    gTasks[taskId].tTest2MonNumber = TEST2_MON_CINDERF;
    gTasks[taskId].tTest2MonState = TEST2_MON_STATE_NONE;
    gTasks[taskId].tTest2MonTimer = 0;
    gTasks[taskId].tTest2MonSpriteId = SPRITE_NONE;
    gTasks[taskId].tTest2MonCryPlayed = FALSE;
    gTasks[taskId].tTest2NextMonFrame = TEST2_MON_INTERVAL_FRAMES;

    gTasks[taskId].func = Task_Test2_Show;
}

static void Task_Test2_Show(u8 taskId)
{
    u16 black = RGB_BLACK;

    /*
     * The display is still disabled here. Prepare the window and
     * palette, but do not enable DISPCNT yet.
     */

    LoadPalette(
        &black,
        0,
        sizeof(black)
    );

    SetGpuReg(
        REG_OFFSET_WIN0H,
        WIN_RANGE(0, DISPLAY_WIDTH)
    );

    SetGpuReg(
        REG_OFFSET_WIN0V,
        WIN_RANGE(
            TEST2_CINEMATIC_BAR_HEIGHT,
            DISPLAY_HEIGHT - TEST2_CINEMATIC_BAR_HEIGHT
        )
    );

    SetGpuReg(
        REG_OFFSET_WININ,
        WININ_WIN0_ALL
    );

    SetGpuReg(
        REG_OFFSET_WINOUT,
        0
    );

    /*
     * Prepare the fade while the display is still hidden.
     */
    BeginNormalPaletteFade(
        PALETTES_ALL,
        0,
        16,
        0,
        RGB_WHITEALPHA
    );

    /*
     * The new palette is transferred during the next VBlank while
     * DISPCNT is still zero.
     */
    TransferPlttBuffer();

    gTasks[taskId].func = Task_Test2_ShowFrame;
}

static void Task_Test2_ShowFrame(u8 taskId)
{
    /*
     * At least one VBlank has now occurred since the Test 2 palette
     * was loaded. The hardware palette is synchronized.
     */

    Test2SetCinematicBars(TEST2_CINEMATIC_BAR_HEIGHT);

    gTasks[taskId].func = Task_Test2_Opening;
}

#undef tTest1WaterFrame
#undef tTest1WaterScroll
#undef tTest1MountainState
#undef tTest1MountainHold

static void Task_Scene1_FadeIn(u8 taskId)
{
BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
SetVBlankCallback(VBlankCB_Intro);
SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
gTasks[taskId].func = Task_Scene1_WaterDrops;
gIntroFrameCounter = 0;
m4aSongNumStart(MUS_INTRO);
ResetSerial();
}

// Task data for Task_Scene1_PanUp
#define tBg2PosHi data[1]
#define tBg2PosLo data[2]
#define tBg1PosHi data[3]
#define tBg1PosLo data[4]
#define tBg3PosHi data[5]
#define tBg3PosLo data[6]

static void Task_Scene1_WaterDrops(u8 taskId)
{
if (gIntroFrameCounter == TIMER_BIG_DROP_START)
gSprites[gTasks[taskId].sBigDropSpriteId].sState = 1;

if (gIntroFrameCounter == TIMER_LOGO_APPEAR)
 CreateTask(Task_BlendLogoIn, 0);

if (gIntroFrameCounter == TIMER_BIG_DROP_FALLS)
 gSprites[gTasks[taskId].sBigDropSpriteId].sState = 2;

if (gIntroFrameCounter == TIMER_LOGO_BLEND_OUT)
 CreateTask(Task_BlendLogoOut, 0);

if (gIntroFrameCounter == TIMER_SMALL_DROP_1)
 CreateWaterDrop(48, 0, 0x400, 5, 0x70, TRUE);

if (gIntroFrameCounter == TIMER_SMALL_DROP_2)
 CreateWaterDrop(200, 60, 0x400, 9, 0x80, TRUE);

if (gIntroFrameCounter == TIMER_SPARKLES)
 CreateTask(Task_CreateSparkles, 0);

if (gIntroFrameCounter > TIMER_SPARKLES)
 {
 gTasks[taskId].tBg2PosHi = 80;
 gTasks[taskId].tBg2PosLo = 0;
 gTasks[taskId].tBg1PosHi = 24;
 gTasks[taskId].tBg1PosLo = 0;
 gTasks[taskId].tBg3PosHi = 40;
 gTasks[taskId].tBg3PosLo = 0;
 gTasks[taskId].func = Task_Scene1_PanUp;
 }

}

#define tDelay data[1]
#define tTimer data[2]
#define tTimerSteps data[3]
#define tNumSparkles data[4]

static void Task_CreateSparkles(u8 taskId)
{
s16 *data = gTasks[taskId].data;

if (tTimer & 1)
 tTimerSteps;

switch (tState)
 {
 case 0:
 CreateSprite(&sSpriteTemplate_Sparkle, sSparkleCoords[tNumSparkles][0], sSparkleCoords[tNumSparkles][1] + tTimerSteps, 0);
 tState++;
 tDelay = 12;
 tNumSparkles++;
 break;
 case 1:
 if (--tDelay == 0)
 tState = 0;
 break;
 }

if (tTimerSteps > 60)
 DestroyTask(taskId);

}

#undef tDelay
#undef tTimer
#undef tTimerSteps
#undef tNumSparkles

#define sTimer data[0]
static void SpriteCB_Sparkle(struct Sprite *sprite)
{
if (++sprite->sTimer == 12)
DestroySprite(sprite);
}
#undef sTimer

static void Task_Scene1_PanUp(u8 taskId)
{
if (gIntroFrameCounter < TIMER_END_PAN_UP)
{
s32 offset;

// Slide bg 2 downward
 offset = (gTasks[taskId].tBg2PosHi << 16) + (u16)gTasks[taskId].tBg2PosLo;
 offset -= 0x6000;
 gTasks[taskId].tBg2PosHi = offset >> 16;
 gTasks[taskId].tBg2PosLo = offset;
 SetGpuReg(REG_OFFSET_BG2VOFS, gTasks[taskId].tBg2PosHi);

// Slide bg 1 downward
 offset = (gTasks[taskId].tBg1PosHi << 16) + (u16)gTasks[taskId].tBg1PosLo;
 offset -= 0x8000;
 gTasks[taskId].tBg1PosHi = offset >> 16;
 gTasks[taskId].tBg1PosLo = offset;
 SetGpuReg(REG_OFFSET_BG1VOFS, gTasks[taskId].tBg1PosHi);

// Slide bg 3 downward
 offset = (gTasks[taskId].tBg3PosHi << 16) + (u16)gTasks[taskId].tBg3PosLo;
 offset -= 0xC000;
 gTasks[taskId].tBg3PosHi = offset >> 16;
 gTasks[taskId].tBg3PosLo = offset;
 SetGpuReg(REG_OFFSET_BG0VOFS, gTasks[taskId].tBg3PosHi);

if (gIntroFrameCounter == TIMER_FLYGON_SILHOUETTE_APPEAR)
 {
 // Show Flygon silhouette
 u8 spriteId = CreateSprite(&sSpriteTemplate_FlygonSilhouette, 120, DISPLAY_HEIGHT, 10);
 gSprites[spriteId].invisible = TRUE;
 }
 }
 else
 {
 if (gIntroFrameCounter > TIMER_END_SCENE_1)
 {
 // Fade to white
 BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITEALPHA);
 gTasks[taskId].func = Task_Scene1_End;
 }
 }

}

static void Task_Scene1_End(u8 taskId)
{
if (gIntroFrameCounter > TIMER_START_SCENE_2)
gTasks[taskId].func = Task_Scene2_Load;
}

static void Task_Scene2_Load(u8 taskId)
{
// FIX: Blackout display during load
SetGpuReg(REG_OFFSET_DISPCNT, 0);

IntroResetGpuRegs();
SetVBlankCallback(NULL);
ResetSpriteData();
FreeAllSpritePalettes();
gIntroCredits_MovingSceneryVBase = 0;
gIntroCredits_MovingSceneryVOffset = 0;
sFlygonYOffset = 0;
LoadIntroPart2Graphics(1);
gTasks[taskId].func = Task_Scene2_CreateSprites;
}

#define tBgAnimTaskId data[0]
#define tPlayerSpriteId data[1]
#define tFlygonSpriteId data[2]
#define tFlygonTimer data[3]

static void Task_Scene2_CreateSprites(u8 taskId)
{
u8 spriteId;

// Load sprite sheets
 if (sIntroCharacterGender == MALE)
 LoadCompressedSpriteSheet(gSpriteSheet_IntroBrendan);
 else
 LoadCompressedSpriteSheet(gSpriteSheet_IntroMay);

LoadCompressedSpriteSheet(gSpriteSheet_IntroBicycle);
 LoadCompressedSpriteSheet(gSpriteSheet_IntroFlygon);

// Load sprite palettes
 for (spriteId = 0; spriteId < ARRAY_COUNT(sSpriteSheet_RunningPokemon) - 1; spriteId++)
 LoadCompressedSpriteSheet(&sSpriteSheet_RunningPokemon[spriteId]);

LoadSpritePalettes(gSpritePalettes_IntroPlayerFlygon);
 LoadSpritePalettes(sSpritePalettes_RunningPokemon);

// Create Pokémon and player sprites
 CreateSprite(&sSpriteTemplate_Manectric, DISPLAY_WIDTH + 32, 128, 0);
 CreateSprite(&sSpriteTemplate_Torchic, DISPLAY_WIDTH + 48, 110, 1);

if (sIntroCharacterGender == MALE)
 spriteId = CreateIntroBrendanSprite(DISPLAY_WIDTH + 32, 100);
 else
 spriteId = CreateIntroMaySprite(DISPLAY_WIDTH + 32, 100);

gSprites[spriteId].callback = SpriteCB_PlayerOnBicycle;
 gSprites[spriteId].anims = sAnims_PlayerBicycle;
 gTasks[taskId].tPlayerSpriteId = spriteId;
 CreateSprite(&sSpriteTemplate_Volbeat, DISPLAY_WIDTH + 32, 80, 4);
 spriteId = CreateIntroFlygonSprite(-64, 60);
 gSprites[spriteId].callback = SpriteCB_Flygon;
 gTasks[taskId].tFlygonSpriteId = spriteId;

// Fade in and start bike ride
 BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
 SetVBlankCallback(VBlankCB_Intro);
 gTasks[taskId].tBgAnimTaskId = CreateBicycleBgAnimationTask(1, 0x4000, 0x400, 0x10);
 SetIntroPart2BgCnt(1);
 gTasks[taskId].func = Task_Scene2_BikeRide;

}

static void Task_Scene2_BikeRide(u8 taskId)
{
u16 offset;

if (gIntroFrameCounter == TIMER_TORCHIC_EXIT)
 {
 // Stop the moving scenery/backgrounds, for when the camera fixes on Torchic
 gIntroCredits_MovingSceneryState = INTROCRED_SCENERY_FROZEN;
 DestroyTask(gTasks[taskId].tBgAnimTaskId);
 }

if (gIntroFrameCounter > TIMER_END_SCENE_2)
 {
 // Fade out to next scene
 BeginNormalPaletteFade(PALETTES_ALL, 8, 0, 16, RGB_WHITEALPHA);
 gTasks[taskId].func = Task_Scene2_End;
 }

// Check for updates to player/flygon sprites
 // These states are for SpriteCB_PlayerOnBicycle and SpriteCB_Flygon respectively
 if (gIntroFrameCounter == TIMER_PLAYER_DRIFT_BACK)
 gSprites[gTasks[taskId].tPlayerSpriteId].sState = 1;
 if (gIntroFrameCounter == TIMER_PLAYER_MOVE_FORWARD)
 gSprites[gTasks[taskId].tPlayerSpriteId].sState = 0;
 if (gIntroFrameCounter == TIMER_FLYGON_ENTER)
 gSprites[gTasks[taskId].tFlygonSpriteId].sState = 1;
 if (gIntroFrameCounter == TIMER_PLAYER_MOVE_BACKWARD)
 gSprites[gTasks[taskId].tPlayerSpriteId].sState = 2;
 if (gIntroFrameCounter == TIMER_PLAYER_HOLD_POSITION)
 gSprites[gTasks[taskId].tPlayerSpriteId].sState = 3;
 if (gIntroFrameCounter == TIMER_PLAYER_EXIT)
 gSprites[gTasks[taskId].tPlayerSpriteId].sState = 4;

// Handle flygon's y movement
 offset = Sin(gTasks[taskId].tFlygonTimer >> 2 & 0x7F, 48);
 sFlygonYOffset = offset;
 if (gTasks[taskId].tFlygonTimer < 512)
 gTasks[taskId].tFlygonTimer++;

// Alternate colors of the trees
 CycleSceneryPalette(0);

}

static void Task_Scene2_End(u8 taskId)
{
if (gIntroFrameCounter > TIMER_START_SCENE_3)
gTasks[taskId].func = Task_Scene3_Load;
}

#define sStateDelay data[1]
#define sNextState data[2]
#define sCosYIdx data[3]
#define sSinXIdx data[4]
#define sSinYIdx data[5]
#define sFig8Loops data[6]

enum {
VOLBEAT_WAIT_ENTER,
VOLBEAT_ENTER,
VOLBEAT_ZIP_BACKWARD,
VOLBEAT_ZIP_DOWN,
VOLBEAT_ZIP_FORWARD,
VOLBEAT_INIT_FIGURE_8,
VOLBEAT_FIGURE_8,
VOLBEAT_EXIT,
VOLBEAT_WAIT_STATE
};

static void SpriteCB_Volbeat(struct Sprite *sprite)
{
sprite->sCosYIdx += 4;
switch (sprite->sState)
{
case VOLBEAT_WAIT_ENTER:
if (sprite->sStateDelay < 180)
break;
sprite->sState;
// fallthrough
case VOLBEAT_ENTER:
sprite->x -= 4;
if (sprite->x == 60)
{
sprite->sState = VOLBEAT_WAIT_STATE;
sprite->sStateDelay = 20;
sprite->sNextState = VOLBEAT_ZIP_BACKWARD;
}
break;
case VOLBEAT_ZIP_BACKWARD:
sprite->x += 8;
sprite->y -= 2;
if (sprite->x == 124)
{
sprite->sState = VOLBEAT_WAIT_STATE;
sprite->sStateDelay = 20;
sprite->sNextState = VOLBEAT_ZIP_DOWN;
}
break;
case VOLBEAT_ZIP_DOWN:
sprite->y += 4;
if (sprite->y == 80)
{
sprite->sState = VOLBEAT_WAIT_STATE;
sprite->sStateDelay = 10;
sprite->sNextState = VOLBEAT_ZIP_FORWARD;
}
break;
case VOLBEAT_ZIP_FORWARD:
sprite->x -= 8;
sprite->y -= 2;
if (sprite->x == 60)
{
sprite->sState = VOLBEAT_WAIT_STATE;
sprite->sStateDelay = 10;
sprite->sNextState = VOLBEAT_INIT_FIGURE_8;
}
break;
case VOLBEAT_INIT_FIGURE_8:
sprite->x += 60;
sprite->sSinXIdx = 0xC0;
sprite->sSinYIdx = 0x80;
sprite->sFig8Loops = 3;
sprite->sState++;
// fallthrough
case VOLBEAT_FIGURE_8:
sprite->x2 = Sin((u8)sprite->sSinXIdx, 0x3C);
sprite->y2 = Sin((u8)sprite->sSinYIdx, 0x14);
sprite->sSinXIdx += 2;
sprite->sSinYIdx += 4;
if ((sprite->sSinXIdx & 0xFF) == 64)
{
sprite->hFlip = FALSE;
if (--sprite->sFig8Loops == 0)
{
sprite->x += sprite->x2;
sprite->x2 = 0;
sprite->sState++;
}
}
break;
case VOLBEAT_EXIT:
sprite->x -= 2;
sprite->y2 = Sin((u8)sprite->sSinYIdx, 0x14);
sprite->sSinYIdx += 4;
if (sprite->x < -16)
DestroySprite(sprite);
break;
case VOLBEAT_WAIT_STATE:
// Wait for state progression, fly idly until then
sprite->y2 = Cos((u8)sprite->sCosYIdx, 2);
if (!--sprite->sStateDelay)
sprite->sState = sprite->sNextState;
break;
}
}

#undef sStateDelay
#undef sNextState
#undef sCosYIdx
#undef sSinXIdx
#undef sSinYIdx
#undef sFig8Loops

#define sMoveTimer data[1]
#define sDelay data[2]

static void SpriteCB_Torchic(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
if (gIntroFrameCounter == TIMER_TORCHIC_ENTER)
{
StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
sprite->sState++;
}
break;
case 1:
if (gIntroFrameCounter == TIMER_PLAYER_HOLD_POSITION)
{
StartSpriteAnim(sprite, TORCHIC_ANIM_WALK);
sprite->sState++;
}
else
{
sprite->sMoveTimer += 64;
if (sprite->sMoveTimer & 0xFF00)
{
sprite->x--;
sprite->sMoveTimer &= 0xFF;
}
}
break;
case 2:
if (gIntroFrameCounter != TIMER_TORCHIC_SPEED_UP)
{
sprite->sMoveTimer += 32;
if (sprite->sMoveTimer & 0xFF00)
{
sprite->x++;
sprite->sMoveTimer &= 0xFF;
}
}
else
{
StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
sprite->sState++;
sprite->sDelay = 80;
}
break;
case 3:
if (--sprite->sDelay)
{
sprite->sMoveTimer += 64;
if (sprite->sMoveTimer & 0xFF00)
{
sprite->x--;
sprite->sMoveTimer &= 0xFF;
}
}
else
{
StartSpriteAnim(sprite, TORCHIC_ANIM_TRIP);
sprite->sState++;
}
break;
case 4:
if (sprite->animEnded)
sprite->x += 4;

if (sprite->x > 336)
 {
 StartSpriteAnim(sprite, TORCHIC_ANIM_RUN);
 sprite->sState++;
 }
 break;
 case 5:
 if (gIntroFrameCounter >= TIMER_TORCHIC_EXIT)
 sprite->x -= 2;
 break;
 }

}

#undef sMoveTimer
#undef sDelay

#define sSinIdx data[1]
#define sCosIdx data[2]

static void SpriteCB_Manectric(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
if (gIntroFrameCounter == TIMER_MANECTRIC_ENTER)
sprite->sState++;
break;
case 1:
sprite->x -= 2;
if (gIntroFrameCounter != TIMER_MANECTRIC_RUN_CIRCULAR)
break;

// Initialize circular pattern running
 sprite->y -= 12;
 sprite->sSinIdx = 0x80;
 sprite->sCosIdx = 0;
 sprite->sState++;
 // fallthrough
 case 2:
 if (sprite->x + sprite->x2 <= -32)
 {
 // Manectric is offscreen now, destroy it
 DestroySprite(sprite);
 }
 else
 {
 // Run in circular pattern
 if ((sprite->sSinIdx & 0xFF) < 64)
 {
 sprite->x2 = Sin((u8)sprite->sSinIdx, 16);
 }
 else
 {
 if ((sprite->sSinIdx & 0xFF) == 64)
 sprite->x -= 48;
 sprite->x2 = Sin((u8)sprite->sSinIdx, 64);
 }
 sprite->sSinIdx++;
 sprite->y2 = Cos((u8)sprite->sCosIdx, 12);
 sprite->sCosIdx++;
 }
 break;
 }

}

#undef sSinIdx
#undef sCosIdx

#define tAlpha data[0]
#define tZoomDiv data[1]
#define tZoomDivSpeed data[2]

static void CopyTest3MountainMap(void)
{
    const u16 *src = sTest3MountainMap;
    u16 *dst = (u16 *)BG_SCREEN_ADDR(TEST1_MOUNTAIN_SCREENBASE);
    u16 row;
    u16 col;

    /*
     * The mountain uses palette bank 1. The map generated by Grit can carry
     * palette-bank bits from the source PNG; replace those bits explicitly
     * so the map cannot accidentally use Test 2's runner palette in bank 0.
     */
    for (row = 0; row < INTRO_BG_SCREEN_HEIGHT; row++)
    {
        for (col = 0; col < INTRO_BG_SCREEN_WIDTH; col++)
        {
            u16 entry = src[row * INTRO_BG_SCREEN_WIDTH + col];
            dst[row * INTRO_BG_SCREEN_WIDTH + col] =
                (entry & 0x0FFF) | (TEST1_MOUNTAIN_PALETTE << 12);
        }
    }
}

static u16 Test3InterpolateColor(u16 from, u16 to, u8 step, u8 duration)
{
    s16 rFrom = from & 0x1F;
    s16 gFrom = (from >> 5) & 0x1F;
    s16 bFrom = (from >> 10) & 0x1F;
    s16 rTo = to & 0x1F;
    s16 gTo = (to >> 5) & 0x1F;
    s16 bTo = (to >> 10) & 0x1F;

    return ((rFrom + ((rTo - rFrom) * step) / duration) & 0x1F)
         | (((gFrom + ((gTo - gFrom) * step) / duration) & 0x1F) << 5)
         | (((bFrom + ((bTo - bFrom) * step) / duration) & 0x1F) << 10);
}

static void Test3FadeMountainToMuted(u8 step)
{
    u16 palette[16];
    u16 i;

    for (i = 0; i < ARRAY_COUNT(palette); i++)
    {
        if (i == 2)
            palette[i] = sTest3MountainOriginalPalette[i];
        else
            palette[i] = Test3InterpolateColor(
                sTest3MountainOriginalPalette[i],
                sTest3MountainMutedPalette[i],
                step,
                16
            );
    }

    LoadPalette(
        palette,
        BG_PLTT_ID(TEST1_MOUNTAIN_PALETTE),
        sizeof(palette)
    );
}

static void Test3SetMountainPaletteMuted(u8 muted)
{
    const u16 *palette = muted ? sTest3MountainMutedPalette : sTest3MountainOriginalPalette;

    /* LoadPalette updates both the faded and unfaded palette buffers. */
    LoadPalette(palette, BG_PLTT_ID(TEST1_MOUNTAIN_PALETTE), sizeof(sTest3MountainOriginalPalette));
}

static void Test3BuildMountainPalettes(void)
{
    u16 i;
    u16 fillColor;

    CpuCopy16(sTest3MountainPal, sTest3MountainOriginalPalette, sizeof(sTest3MountainOriginalPalette));
    fillColor = sTest3MountainOriginalPalette[1];

    for (i = 0; i < ARRAY_COUNT(sTest3MountainMutedPalette); i++)
    {
        /* Palette index 2 is the background color and must remain unchanged. */
        if (i == 2)
            sTest3MountainMutedPalette[i] = sTest3MountainOriginalPalette[i];
        else
            sTest3MountainMutedPalette[i] = fillColor;
    }
}

static void Task_Test3_LoadMountains(u8 taskId)
{
    /* Start from a clean, hidden display so Test 2 graphics cannot leak in. */
    PrepareIntroSceneTransition();
    ResetPaletteFade();
    ResetSpriteData();
    FreeAllSpritePalettes();
    sTest3MountainLightningMode = FALSE;

    DmaFill16(3, 0, (void *)VRAM, VRAM_SIZE);
    DmaFill32(3, 0, (void *)OAM, OAM_SIZE);
    DmaFill16(3, 0, (void *)(PLTT + 2), PLTT_SIZE - 2);

    /* Load the 4bpp mountain tiles, map, and palette into BG1. */
    CpuCopy16(
        sTest3MountainGfx,
        (void *)BG_CHAR_ADDR(TEST1_MOUNTAIN_CHARBASE),
        sizeof(sTest3MountainGfx)
    );
    CopyTest3MountainMap();
    LoadPalette(
        sTest3MountainPal,
        BG_PLTT_ID(TEST1_MOUNTAIN_PALETTE),
        sizeof(sTest3MountainOriginalPalette)
    );
    Test3BuildMountainPalettes();
    Test3SetMountainPaletteMuted(TRUE);

    SetGpuReg(
        REG_OFFSET_BG1CNT,
        BGCNT_PRIORITY(1)
        | BGCNT_CHARBASE(TEST1_MOUNTAIN_CHARBASE)
        | BGCNT_SCREENBASE(TEST1_MOUNTAIN_SCREENBASE)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

    /* Restore VBlank only after the new scene's VRAM and palette are ready. */
    SetVBlankCallback(VBlankCB_Intro);
    SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0 | DISPCNT_BG1_ON);

    /* Fade in the grey mountain, then hold it for about three seconds. */
    BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_BLACK);
    gTasks[taskId].data[0] = 0;
    gTasks[taskId].data[1] = 60;
    gTasks[taskId].func = Task_Test3_HoldMountain;
}

static void Task_Test3_LoadMountainLightning(u8 taskId)
{
    LoadCompressedSpriteSheetUsingHeap(sSpriteSheet_Lightning);
    LoadSpritePalettes(sSpritePalette_Lightning);

    SetGpuReg(
        REG_OFFSET_DISPCNT,
        DISPCNT_MODE_0
        | DISPCNT_OBJ_1D_MAP
        | DISPCNT_BG1_ON
        | DISPCNT_OBJ_ON
    );

    sTest3MountainLightningMode = TRUE;

    gTasks[taskId].data[0] = 0; /* lightning state */
    gTasks[taskId].data[1] = 0; /* mountain currently colored */
    gTasks[taskId].data[5] = 0; /* mountain gray fade timer */
    gTasks[taskId].data[6] = TEST3_LIGHTNING_FIRST_DELAY;
    gTasks[taskId].data[7] = 0; /* lightning timer */

    gTasks[taskId].func = Task_Scene3_Lightning;
}

/* Mountain-only Test 3: grey mountain, original lightning strikes, fade to white. */
static void Task_Test3_HoldMountain(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    switch (task->data[0])
    {
    case 0: /* Wait until the fade-in has finished. */
        if (!gPaletteFade.active)
            task->data[0] = 1;
        break;

    case 1: /* Hold the grey mountain before the first strike. */
        if (task->data[1] > 0)
            task->data[1]--;
        else
            Task_Test3_LoadMountainLightning(taskId);
        break;

    case 2: /* Leave the display white after the final fade completes. */
        if (!gPaletteFade.active)
        {
            sTest3MountainLightningMode = FALSE;
            DestroyTask(taskId);
        }
        break;
    }
}

static void Task_Scene3_Load(u8 taskId)
{
// FIX: Blackout display during load
SetGpuReg(REG_OFFSET_DISPCNT, 0);

IntroResetGpuRegs();
DecompressDataWithHeaderVram(sIntroPokeball_Gfx, (void *)VRAM);
DecompressDataWithHeaderVram(sIntroPokeball_Tilemap, (void *)(BG_CHAR_ADDR(1)));
LoadPalette(sIntroPokeball_Pal, BG_PLTT_ID(0), sizeof(sIntroPokeball_Pal));
gTasks[taskId].tAlpha = 0;
gTasks[taskId].tZoomDiv = 0;
gTasks[taskId].tZoomDivSpeed = 0;
gTasks[taskId].data[3] = 0;
PanFadeAndZoomScreen(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, 0, 0);
ResetSpriteData();
FreeAllSpritePalettes();
BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(3) | BGCNT_CHARBASE(0) | BGCNT_SCREENBASE(8) | BGCNT_256COLOR | BGCNT_AFF256x256);
SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1 | DISPCNT_OBJ_1D_MAP | DISPCNT_BG2_ON | DISPCNT_OBJ_ON);
gTasks[taskId].func = Task_Scene3_SpinPokeball;
gIntroFrameCounter = 0;
m4aSongNumStart(MUS_INTRO_BATTLE);
}
static void Task_Scene3_SpinPokeball(u8 taskId)
{
gTasks[taskId].tAlpha += 0x400;

if (gTasks[taskId].tZoomDiv <= 0x6BF)
 {
 gTasks[taskId].tZoomDiv += gTasks[taskId].tZoomDivSpeed;
 gTasks[taskId].tZoomDivSpeed += 2;
 }
 else
 {
 gTasks[taskId].func = Task_Scene3_WaitGroudon;
 }

PanFadeAndZoomScreen(DISPLAY_WIDTH / 2, DISPLAY_HEIGHT / 2, SAFE_DIV(0x10000, gTasks[taskId].tZoomDiv), gTasks[taskId].tAlpha);

if (gIntroFrameCounter == TIMER_POKEBALL_FADE)
 BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITEALPHA);

}

#undef tAlpha
#undef tZoomDiv
#undef tZoomDivSpeed

static void Task_Scene3_WaitGroudon(u8 taskId)
{
if (gIntroFrameCounter > TIMER_START_LEGENDARIES)
gTasks[taskId].func = Task_Scene3_LoadGroudon;
}

static void Task_Test2_Wait(u8 taskId)
{
    if (!gPaletteFade.active)
    {
        gTasks[taskId].tTest2State = TEST2_STATE_PAN;
        gTasks[taskId].tTest2Timer = 0;
        gTasks[taskId].tTest2Displacement = TEST2_START_VOFS;

        Test2ApplySharedDisplacement(TEST2_START_VOFS);
        gTasks[taskId].func = Task_Test2_Opening;
    }
}

static void Task_Scene3_LoadGroudon(u8 taskId)
{
if (!gPaletteFade.active)
{
// FIX: Blackout display during load
SetGpuReg(REG_OFFSET_DISPCNT, 0);

IntroResetGpuRegs();
ResetSpriteData();
FreeAllSpritePalettes();
gReservedSpritePaletteCount = 8;
DecompressDataWithHeaderVram(gIntroGroudon_Gfx, (void *)VRAM);
DecompressDataWithHeaderVram(gIntroGroudon_Tilemap, (void *)(BG_CHAR_ADDR(3)));
DecompressDataWithHeaderVram(gIntroLegendBg_Gfx, (void *)(BG_CHAR_ADDR(1)));
DecompressDataWithHeaderVram(gIntroGroudonBg_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
LoadCompressedSpriteSheetUsingHeap(&gBattleAnimTable[GET_TRUE_SPRITE_INDEX(ANIM_TAG_ROCKS)].pic);
LoadSpritePalette(&gBattleAnimTable[GET_TRUE_SPRITE_INDEX(ANIM_TAG_ROCKS)].palette);
CpuCopy16(gIntro3Bg_Pal, gPlttBufferUnfaded, sizeof(gIntro3Bg_Pal));
gTasks[taskId].func = Task_Scene3_InitGroudonBg;
}
}

#define tWinPos data[0]
#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom data[3]

static void Task_Scene3_InitGroudonBg(u8 taskId)
{
SetGpuReg(REG_OFFSET_WIN0H, DISPLAY_WIDTH);
SetGpuReg(REG_OFFSET_WIN0V, DISPLAY_HEIGHT);
SetGpuReg(REG_OFFSET_WININ, WININ_WIN0_ALL);
SetGpuReg(REG_OFFSET_WINOUT, 0);
SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(0)
| BGCNT_CHARBASE(0)
| BGCNT_SCREENBASE(24)
| BGCNT_256COLOR
| BGCNT_WRAP
| BGCNT_AFF512x512);
SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(1)
| BGCNT_CHARBASE(1)
| BGCNT_SCREENBASE(28)
| BGCNT_16COLOR
| BGCNT_TXT256x256);
SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_1
| DISPCNT_OBJ_1D_MAP
| DISPCNT_BG1_ON
| DISPCNT_BG2_ON
| DISPCNT_OBJ_ON
| DISPCNT_WIN0_ON);
BeginNormalPaletteFade(PALETTES_ALL, 0, 16, 0, RGB_WHITEALPHA);
gTasks[taskId].tWinPos = 0;
gTasks[taskId].tScreenX = 0xFFA0;
gTasks[taskId].tScreenY = 0xFF51;
gTasks[taskId].tZoom = 0x100;
PanFadeAndZoomScreen(gTasks[taskId].tScreenX, gTasks[taskId].tScreenY, gTasks[taskId].tZoom, 0);
gTasks[taskId].func = Task_Scene3_NarrowWindow;
}

// Before the Groudon scene starts, the black top/bottom edges of the screen
// come inward for a more 'cinematic' look
#define NARROW_HEIGHT 32
static void Task_Scene3_NarrowWindow(u8 taskId)
{
if (gTasks[taskId].tWinPos != NARROW_HEIGHT)
{
gTasks[taskId].tWinPos += 4;
SetGpuReg(REG_OFFSET_WIN0V, (gTasks[taskId].tWinPos * 256) - (gTasks[taskId].tWinPos - DISPLAY_HEIGHT));
}
else
{
SetGpuReg(REG_OFFSET_WIN0V, WIN_RANGE(NARROW_HEIGHT, DISPLAY_HEIGHT - NARROW_HEIGHT));
gTasks[taskId].func = Task_Scene3_EndNarrowWindow;
}
}
#undef tWinPos
#undef tScreenX
#undef tScreenY
#undef tZoom

static void Task_Scene3_EndNarrowWindow(u8 taskId)
{
gTasks[taskId].func = Task_Scene3_StartGroudon;
}

static void Task_Scene3_StartGroudon(u8 taskId)
{
gTasks[taskId].tState = 0;
gTasks[taskId].func = Task_Scene3_Groudon;
ScanlineEffect_InitWave(0, DISPLAY_HEIGHT, 4, 4, 1, SCANLINE_EFFECT_REG_BG1HOFS, FALSE);
}

#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom data[3]
#define tYShake data[4]
#define tTimer data[5]
#define tDelay data[6]
#define tTrigIdx data[6] // Re-used
#define tPalIdx data[7]

// Treats gIntro3Bg_Pal as u8* and iterates by 1
#define INTRO3_RAW_PTR(palId)(((void *) &gIntro3Bg_Pal) + palId)

static void Task_Scene3_Groudon(u8 taskId)
{
s16 *data = gTasks[taskId].data;

tTimer++;
 if ((tState >= 1 && tState <= 7) && tTimer % 2 == 0)
 tYShake ^= 3;
 PanFadeAndZoomScreen(tScreenX, tScreenY + tYShake, tZoom, 0);
 switch (tState)
 {
 case 0:
 tScreenX += 16;
 if (tScreenX == 160)
 {
 tState++;
 tDelay = 2;
 tPalIdx = 0x1E2;
 CreateGroudonRockSprites(taskId);
 }
 break;
 case 1:
 if (--tDelay == 0)
 {
 tDelay = 2;
 CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
 tPalIdx += 2;
 if (tPalIdx == 0x1EC)
 tState++;
 }
 break;
 case 2:
 if (--tDelay == 0)
 {
 tDelay = 2;
 tState++;
 }
 break;
 case 3:
 if (--tDelay == 0)
 {
 tDelay = 2;
 CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(1) + 15], PLTT_SIZEOF(1));
 tPalIdx -= 2;
 if (tPalIdx == 0x1E0)
 {
 tDelay = 8;
 tState++;
 }
 }
 break;
 case 4:
 if (--tDelay == 0)
 {
 tScreenX = -96;
 tScreenY = 169;
 tDelay = 3;
 tState++;
 }
 break;
 case 5:
 if (--tDelay == 0)
 {
 tScreenX = 80;
 tScreenY = 41;
 tDelay = 16;
 if (IsSpeciesEnabled(SPECIES_GROUDON))
 PlayCryInternal(SPECIES_GROUDON, 0, 100, CRY_PRIORITY_NORMAL, CRY_MODE_NORMAL);
 tState++;
 }
 break;
 case 6:
 if (--tDelay == 0)
 {
 tScreenX = 80;
 tScreenY = 40;
 tState++;
 }
 break;
 case 7:
 tScreenX += 4;
 tScreenY += 4;
 tTrigIdx += 0x666;
 tZoom = Sin((tTrigIdx & 0xFF00) >> 8, 64) + 256;
 if (tScreenX == 120)
 {
 BeginNormalPaletteFade(PALETTES_ALL & ~1, 3, 0, 16, RGB_WHITE);
 tZoom = 256;
 tYShake = 0;
 tState++;
 }
 break;
 case 8:
 if (tZoom)
 tZoom -= 8;
 else
 tState++;
 break;
 case 9:
 if (!gPaletteFade.active)
 {
 gTasks[taskId].func = Task_Scene3_LoadKyogre;
 gScanlineEffect.state = 3;
 }
 break;
 }

}

#undef tScreenX
#undef tScreenY
#undef tZoom
#undef tYShake
#undef tTimer
#undef tDelay
#undef tTrigIdx
#undef tPalIdx

#define sRockId data[1]
#define sSpeed data[2]
#define sTimer data[3]
#define sTaskId data[4]

static void CreateGroudonRockSprites(u8 taskId)
{
int i;
u8 spriteId;

for (i = 0; i < (int)ARRAY_COUNT(sGroudonRockData); i++)
 {
 spriteId = CreateSprite(&gAncientPowerRockSpriteTemplate, sGroudonRockData[i][0], DISPLAY_HEIGHT, i);
 gSprites[spriteId].callback = SpriteCB_GroudonRocks;
 gSprites[spriteId].oam.priority = 0;
 gSprites[spriteId].sRockId = i;
 gSprites[spriteId].sTaskId = taskId;
 StartSpriteAnim(&gSprites[spriteId], sGroudonRockData[i][1]);
 }

}

static void SpriteCB_GroudonRocks(struct Sprite *sprite)
{
// Introduce some wobble to the floating
sprite->sTimer++;
if (sprite->sTimer % 2 == 0)
sprite->y2 ^= 3;

switch (sprite->sState)
 {
 case 0:
 // Rock floats up
 sprite->sSpeed += sGroudonRockData[sprite->sRockId][2];
 sprite->y -= (sprite->sSpeed & 0xFF00) >> 8;
 sprite->sSpeed &= 0xFF;

// Check if Groudon scene is ending
 if (gTasks[sprite->sTaskId].tState > 7)
 sprite->sState++;
 break;
 case 1:
 // Scene zooms in, move rock offscreen
 if (sprite->x < DISPLAY_WIDTH / 2)
 sprite->x -= 2;
 else
 sprite->x += 2;

if (sprite->y < DISPLAY_HEIGHT / 2)
 sprite->y -= 2;
 else
 sprite->y += 2;
 break;
 }

}

#undef sRockId
#undef sSpeed
#undef sTimer
#undef sTaskId

#define tScreenX data[1]
#define tScreenY data[2]
#define tZoom data[3]
#define tDelay data[6]
#define tTrigIdx data[6] // Re-used
#define tPalIdx data[7]

static void Task_Scene3_LoadKyogre(u8 taskId)
{
// FIX: Blackout display during load
SetGpuReg(REG_OFFSET_DISPCNT, 0);

ResetSpriteData();
DecompressDataWithHeaderVram(gIntroKyogre_Gfx, (void *)VRAM);
DecompressDataWithHeaderVram(gIntroKyogre_Tilemap, (void *)(BG_CHAR_ADDR(3)));
DecompressDataWithHeaderVram(gIntroKyogreBg_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
LoadCompressedSpriteSheet(sSpriteSheet_Bubbles);
LoadSpritePalette(sSpritePalette_Bubbles);
BeginNormalPaletteFade(PALETTES_ALL & ~1, 0, 16, 0, RGB_WHITEALPHA);
gTasks[taskId].func = Task_Scene3_Kyogre;
gTasks[taskId].tState = 0;
gTasks[taskId].tScreenX = 336;
gTasks[taskId].tScreenY = 80;
gTasks[taskId].tDelay = 16;
gTasks[taskId].tZoom = 256;
PanFadeAndZoomScreen(gTasks[taskId].tScreenX, gTasks[taskId].tScreenY, gTasks[taskId].tZoom, 0);
ScanlineEffect_InitWave(0, DISPLAY_HEIGHT, 4, 4, 1, SCANLINE_EFFECT_REG_BG1VOFS, FALSE);
}

static void Task_Scene3_Kyogre(u8 taskId)
{
s16 *data = gTasks[taskId].data;

PanFadeAndZoomScreen(tScreenX, tScreenY, tZoom, 0);

switch (tState)
 {
 case 0:
 if (--tDelay != 0)
 break;
 tState++;
 case 1:
 tTrigIdx += 4;
 gTasks[taskId].tScreenX = 344 - Sin(tTrigIdx, 0x100);
 gTasks[taskId].tScreenY = 84 - Cos(tTrigIdx, 0x40);
 if (tTrigIdx == 64)
 {
 tDelay = 0x19;
 tPalIdx = 1;
 tState++;
 CreateKyogreBubbleSprites_Body(0);
 }
 break;
 case 2:
 if (--tDelay == 0)
 {
 gTasks[taskId].tScreenX += 256;
 gTasks[taskId].tScreenY -= 258;
 tDelay = 8;
 tState++;
 CreateKyogreBubbleSprites_Body(0);
 CreateKyogreBubbleSprites_Fins();
 }
 break;
 case 3:
 if (--tDelay == 0)
 {
 gTasks[taskId].tScreenX -= 256;
 gTasks[taskId].tScreenY += 258;
 tDelay = 8;
 tState++;
 }
 break;
 case 4:
 if (--tDelay == 0)
 {
 gTasks[taskId].tScreenY -= 252;
 tDelay = 8;
 tState++;
 }
 break;
 case 5:
 if (--tDelay == 0)
 {
 gTasks[taskId].tScreenY += 252;
 if (tPalIdx != 0)
 {
 tDelay = 12;
 tPalIdx--;
 tState = 2;
 }
 else
 {
 tDelay = 1;
 tState++;
 if (IsSpeciesEnabled(SPECIES_KYOGRE))
 PlayCryInternal(SPECIES_KYOGRE, 0, 120, CRY_PRIORITY_NORMAL, CRY_MODE_NORMAL);
 }
 }
 break;
 case 6:
 if (--tDelay == 0)
 {
 tDelay = 4;
 tPalIdx = 0x1EA;
 tState++;
 }
 break;
 case 7:
 if (--tDelay == 0)
 {
 tDelay = 4;
 CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(2) + 15], PLTT_SIZEOF(1));
 tPalIdx -= 2;
 if (tPalIdx == 0x1E0)
 tState++;
 }
 break;
 case 8:
 if (--tDelay == 0)
 {
 tDelay = 4;
 tPalIdx = 0x1E2;
 tState++;
 }
 break;
 case 9:
 if (--tDelay == 0)
 {
 tDelay = 4;
 CpuCopy16(INTRO3_RAW_PTR(tPalIdx), &gPlttBufferFaded[BG_PLTT_ID(2) + 15], PLTT_SIZEOF(1));
 tPalIdx += 2;
 if (tPalIdx == 0x1EE)
 {
 tDelay = 16;
 tState++;
 }
 }
 break;
 case 10:
 if (--tDelay == 0)
 {
 tTrigIdx = 0;
 tState++;
 CreateKyogreBubbleSprites_Body(taskId);
 }
 break;
 case 11:
 tTrigIdx += 4;
 tZoom -= 8;
 gTasks[taskId].tScreenX = Sin(tTrigIdx, 0x3C) + 88;
 if (tTrigIdx == 64)
 {
 BeginNormalPaletteFade(PALETTES_ALL & ~1, 3, 0, 16, RGB_WHITE);
 tState++;
 }
 break;
 case 12:
 tTrigIdx += 4;
 tZoom -= 8;
 gTasks[taskId].tScreenX = Sin(tTrigIdx, 0x14) + 128;
 if (tTrigIdx == 128)
 tState++;
 break;
 case 13:
 if (!gPaletteFade.active)
 {
 gTasks[taskId].func = Task_Scene3_LoadClouds1;
 gScanlineEffect.state = 3;
 }
 break;
 }

}

#undef tScreenX
#undef tScreenY
#undef tZoom
#undef tDelay
#undef tTrigIdx
#undef tPalIdx

#define sSinIdx data[1]
#define sBaseY data[2]
#define sTaskId data[5]
#define sDelay data[6]
#define sUnk data[7] // Never read

// taskId is used inconsistently for these two functions.
// The sprite callback for the bubbles will always read it, unless delay is 0 to
// start (it never is), but the first function is often passed 0 instead of a
// taskId, and the second function doesn't take/assign a taskId at all.
// The only time an actual taskId is given is when it actually needs the
// result of reading it, to zoom in at the end of the scene.

// Creates bubbles at positions spread across Kyogre's body
static void CreateKyogreBubbleSprites_Body(u8 taskId)
{
int i;
u8 spriteId;

for (i = 0; i < NUM_BUBBLES_IN_SET; i++)
 {
 spriteId = CreateSprite(&sSpriteTemplate_Bubbles,
 sKyogreBubbleData[i][0],
 sKyogreBubbleData[i][1],
 i);
 gSprites[spriteId].invisible = TRUE;
 gSprites[spriteId].sTaskId = taskId;
 gSprites[spriteId].sDelay = sKyogreBubbleData[i][2];
 gSprites[spriteId].sUnk = 64;
 }

}

// Creates bubbles at positions around Kyogre's fins, for when it's moving them
static void CreateKyogreBubbleSprites_Fins(void)
{
int i;
u8 spriteId;

for (i = 0; i < NUM_BUBBLES_IN_SET; i++)
 {
 spriteId = CreateSprite(&sSpriteTemplate_Bubbles,
 sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][0],
 sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][1],
 i);
 gSprites[spriteId].invisible = TRUE;

#ifdef BUGFIX
gSprites[spriteId].sDelay = sKyogreBubbleData[i + NUM_BUBBLES_IN_SET][2];
#else
gSprites[spriteId].sDelay = sKyogreBubbleData[i][2]; // Using the wrong set of delays here
#endif
gSprites[spriteId].sUnk = 64;
}
}

static void SpriteCB_KyogreBubbles(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
if (sprite->sDelay == 0)
{
// Animation has started, float bubbles up
sprite->sSinIdx = (sprite->sSinIdx + 11) & 0xFF;
sprite->x2 = Sin(sprite->sSinIdx, 4);
sprite->sBaseY += 48;
sprite->y2 = -(sprite->sBaseY >> 8);
if (sprite->animEnded)
DestroySprite(sprite);
}
else if (--sprite->sDelay == 0)
{
// Start bubble animation after delay has finished
StartSpriteAnim(sprite, 0);
sprite->invisible = FALSE;
}

// Check if Kyogre scene is ending
 // For all but the last bubbles, sTaskId isn't actually set
 if (gTasks[sprite->sTaskId].tState > 11)
 sprite->sState++;
 break;
 case 1:
 // Scene zooms in, move bubbles offscreen
 if (sprite->x < DISPLAY_WIDTH / 2)
 sprite->x -= 3;
 else
 sprite->x += 3;

if (sprite->y < DISPLAY_HEIGHT / 2)
 sprite->y -= 3;
 else
 sprite->y += 3;

if ((u16)(sprite->y - 20) > DISPLAY_HEIGHT - 20)
 DestroySprite(sprite);
 break;
 }

}

#undef sSinIdx
#undef sBaseY
#undef sTaskId
#undef sDelay
#undef sUnk

static void Task_Scene3_LoadClouds1(u8 taskId)
{
SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_TGT1_BG0
| BLDCNT_TGT1_BG1
| BLDCNT_TGT1_BG2
| BLDCNT_EFFECT_LIGHTEN);
SetGpuReg(REG_OFFSET_BLDALPHA, BLDALPHA_BLEND(31, 31));
SetGpuReg(REG_OFFSET_BLDY, 31);
SetGpuReg(REG_OFFSET_BG0CNT, BGCNT_PRIORITY(0)
| BGCNT_CHARBASE(0)
| BGCNT_SCREENBASE(24)
| BGCNT_16COLOR
| BGCNT_TXT512x256);
SetGpuReg(REG_OFFSET_BG1CNT, BGCNT_PRIORITY(0)
| BGCNT_CHARBASE(1)
| BGCNT_SCREENBASE(26)
| BGCNT_16COLOR
| BGCNT_TXT512x256);
SetGpuReg(REG_OFFSET_BG2CNT, BGCNT_PRIORITY(2)
| BGCNT_CHARBASE(1)
| BGCNT_SCREENBASE(28)
| BGCNT_16COLOR
| BGCNT_TXT256x256);
SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
| DISPCNT_OBJ_1D_MAP
| DISPCNT_BG0_ON
| DISPCNT_BG1_ON
| DISPCNT_BG2_ON
| DISPCNT_OBJ_ON
| DISPCNT_WIN0_ON);
SetGpuReg(REG_OFFSET_BG0HOFS, 80);
SetGpuReg(REG_OFFSET_BG0VOFS, 0);
SetGpuReg(REG_OFFSET_BG1HOFS, -80);
SetGpuReg(REG_OFFSET_BG1VOFS, 0);
SetGpuReg(REG_OFFSET_BG2HOFS, 0);
SetGpuReg(REG_OFFSET_BG2VOFS, 0);
DecompressDataWithHeaderVram(gIntroClouds_Gfx, (void *)VRAM);
DecompressDataWithHeaderVram(gIntroClouds_Gfx, (void *)(BG_CHAR_ADDR(1)));
DecompressDataWithHeaderVram(gIntroCloudsSun_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
gTasks[taskId].func = Task_Scene3_LoadClouds2;
}

static void Task_Scene3_LoadClouds2(u8 taskId)
{
DecompressDataWithHeaderVram(gIntroCloudsLeft_Tilemap, (void *)(BG_CHAR_ADDR(3)));
DecompressDataWithHeaderVram(gIntroCloudsRight_Tilemap, (void *)(BG_SCREEN_ADDR(26)));
gTasks[taskId].func = Task_Scene3_InitClouds;
}

#define tCloudPos data[6]

static void Task_Scene3_InitClouds(u8 taskId)
{
SetGpuReg(REG_OFFSET_BLDCNT, 0);
SetGpuReg(REG_OFFSET_BLDALPHA, 0);
SetGpuReg(REG_OFFSET_BLDY, 0);
gTasks[taskId].func = Task_Scene3_Clouds;
gTasks[taskId].tState = 0;
gTasks[taskId].tCloudPos = 16;
}

// Clouds coming in from the sides before Rayquaza appears
static void Task_Scene3_Clouds(u8 taskId)
{
s16 *data = gTasks[taskId].data;

// Left clouds are on BG0, right clouds are on BG1
 SetGpuReg(REG_OFFSET_BG0HOFS, (tCloudPos >> 8));
 SetGpuReg(REG_OFFSET_BG1HOFS, -(tCloudPos >> 8));

switch (tState)
 {
 case 0:
 // Cloud position is used briefly as a delay, before
 // the scene has faded in from white
 if (--tCloudPos == 0)
 {
 // Start fade in from white, set cloud starting positions
 BeginNormalPaletteFade(PALETTES_ALL & ~1, 0, 16, 0, RGB_WHITEALPHA);
 tCloudPos = 80 << 8;
 tState++;
 }
 break;
 case 1:
 // Start fading out
 if (tCloudPos == 40 << 8)
 BeginNormalPaletteFade(PALETTES_BG & ~1, 3, 0, 16, RGB(9, 10, 10));

// Move clouds inward toward each other
 if (tCloudPos != 0)
 tCloudPos -= 128;
 else if (!gPaletteFade.active)
 gTasks[taskId].func = Task_Scene3_LoadLightning;
 break;
 }

}
#undef tCloudPos

#define tDelay data[6]
#define tTimer data[7]

static void Task_Scene3_LoadLightning(u8 taskId)
{
DecompressDataWithHeaderVram(gIntroRayquaza_Tilemap, (void *)(BG_SCREEN_ADDR(28)));
DecompressDataWithHeaderVram(gIntroRayquazaClouds_Tilemap, (void *)(BG_CHAR_ADDR(3)));
DecompressDataWithHeaderVram(gIntroRayquaza_Gfx, (void *)(BG_CHAR_ADDR(1)));
DecompressDataWithHeaderVram(gIntroRayquazaClouds_Gfx, (void *)VRAM);
SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
| DISPCNT_OBJ_1D_MAP
| DISPCNT_BG0_ON
| DISPCNT_BG2_ON
| DISPCNT_OBJ_ON
| DISPCNT_WIN0_ON);
gTasks[taskId].func = Task_Scene3_Lightning;
gTasks[taskId].tState = 0;
gTasks[taskId].tDelay = 1;
gTasks[taskId].tTimer = 0;
LoadCompressedSpriteSheetUsingHeap(sSpriteSheet_Lightning);
LoadSpritePalettes(sSpritePalette_Lightning);
}

static bool8 Test3LightningSpritesAreVisible(struct Task *task)
{
    u8 i;
    u8 spriteId;
    bool8 visible = FALSE;

    for (i = 0; i < 3; i++)
    {
        spriteId = task->data[2 + i];

        if (spriteId < MAX_SPRITES
            && gSprites[spriteId].inUse
            && !gSprites[spriteId].invisible)
        {
            visible = TRUE;
            break;
        }
    }

    return visible;
}

static void Task_Scene3_Lightning(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *data = task->data;
    u8 spriteId;

    if (sTest3MountainLightningMode
        && task->data[1] != 0
        && task->data[5] == 0
        && !Test3LightningSpritesAreVisible(task))
    {
        task->data[1] = 0;
        task->data[5] = 1; /* Begin the 16-frame fade to gray. */
    }

    if (sTest3MountainLightningMode && task->data[5] != 0)
    {
        Test3FadeMountainToMuted(task->data[5]);

        if (++task->data[5] > 16)
            task->data[5] = 0;
    }

    switch (tState)
    {
    case 0:
        if (--tDelay == 0)
        {
            Test3SetMountainPaletteMuted(FALSE);
            task->data[1] = 1;

            task->data[2] = CreateSprite(
                &sSpriteTemplate_Lightning,
                200,
                16,
                0
            );

            task->data[3] = CreateSprite(
                &sSpriteTemplate_Lightning,
                200,
                48,
                1
            );
            if (task->data[3] != MAX_SPRITES)
                StartSpriteAnim(&gSprites[task->data[3]], 1);

            task->data[4] = CreateSprite(
                &sSpriteTemplate_Lightning,
                200,
                80,
                2
            );
            if (task->data[4] != MAX_SPRITES)
                StartSpriteAnim(&gSprites[task->data[4]], 2);

            tState = 1;
            tDelay = TEST3_LIGHTNING_SECOND_DELAY;
        }
        break;

    case 1:
        if (--tDelay == 0)
        {
            Test3SetMountainPaletteMuted(FALSE);
            task->data[1] = 1;

            task->data[2] = CreateSprite(
                &sSpriteTemplate_Lightning,
                40,
                16,
                0
            );

            task->data[3] = CreateSprite(
                &sSpriteTemplate_Lightning,
                40,
                48,
                1
            );
            if (task->data[3] != MAX_SPRITES)
                StartSpriteAnim(&gSprites[task->data[3]], 1);

            task->data[4] = CreateSprite(
                &sSpriteTemplate_Lightning,
                40,
                80,
                2
            );
            if (task->data[4] != MAX_SPRITES)
                StartSpriteAnim(&gSprites[task->data[4]], 2);

            tState = 2;
            tDelay = TEST3_LIGHTNING_FADE_DELAY;
        }
        break;

    case 2:
        if (--tDelay == 0)
        {
            /*
             * Match the original scene's return to the unlit state.
             * This keeps the mountain gray before the white transition.
             */
            Test3SetMountainPaletteMuted(TRUE);
            task->data[1] = 0;
            sTest3MountainLightningMode = FALSE;

            BeginNormalPaletteFade(
                PALETTES_ALL,
                0,
                0,
                16,
                RGB_WHITEALPHA
            );

            task->data[0] = 3;
            task->func = Task_Test3_LoadLegend;
        }
        break;
    }
}

static void Task_Test3_LoadLegend(u8 taskId)
{
    struct Task *task = &gTasks[taskId];

    if (gPaletteFade.active)
        return;

    SetGpuReg(REG_OFFSET_DISPCNT, 0);
    SetHBlankCallback(NULL);
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();

    CpuCopy16(sLegendBgGfx, (void *)BG_CHAR_ADDR(0), sizeof(sLegendBgGfx));
    CpuCopy16(sLegendBgMap0, (void *)BG_SCREEN_ADDR(8), sizeof(sLegendBgMap0));
    LoadPalette(sLegendBgPal, BG_PLTT_ID(0), sizeof(sLegendBgPal));

    SetGpuReg(
        REG_OFFSET_BG0CNT,
        BGCNT_PRIORITY(0)
        | BGCNT_CHARBASE(0)
        | BGCNT_SCREENBASE(8)
        | BGCNT_16COLOR
        | BGCNT_TXT256x256
    );

    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);

    task->data[0] = 0; // state
    task->data[1] = 0; // timer
    task->data[2] = 0; // HOFS phase
    task->data[3] = 0; // unused

    SetGpuReg(
        REG_OFFSET_DISPCNT,
        DISPCNT_MODE_0
        | DISPCNT_BG0_ON
    );

    BeginNormalPaletteFade(
        PALETTES_ALL,
        0,
        16,
        0,
        RGB_BLACK
    );

    task->func = Task_Test3_Legend;
}

static void Task_Test3_Legend(u8 taskId)
{
    struct Task *task = &gTasks[taskId];
    s16 *data = task->data;

    switch (data[0])
    {
    case 0: // image 0, initial hold
        if (++data[1] >= 60)
        {
            data[0] = 1;
            data[1] = 0;
        }
        break;

    case 1: // soft side-to-side shake
        data[2] += 4;
        SetGpuReg(REG_OFFSET_BG0HOFS, Sin(data[2], 2) & 0x1FF);

        if (++data[1] >= 48)
        {
            data[0] = 2;
            data[1] = 0;
        }
        break;

    case 2: // pause
        if (++data[1] >= 36)
        {
            data[0] = 3;
            data[1] = 0;
            data[2] = 0; /* Restart the shake phase from the beginning. */
        }
        break;

    case 3: // same shake as the first, 1.5x amplitude
        data[2] += 4;
        SetGpuReg(
            REG_OFFSET_BG0HOFS,
            Sin(data[2], 3) & 0x1FF
        );

        if (++data[1] >= 48)
        {
            SetGpuReg(REG_OFFSET_BG0HOFS, 0);

            data[0] = 4;
            data[1] = 0;
        }
        break;

    case 4: /* pause before switching image */
        if (++data[1] >= 36)
        {
            CpuCopy16(
                sLegendBgMap1,
                (void *)BG_SCREEN_ADDR(8),
                sizeof(sLegendBgMap1)
            );

            data[0] = 5;
            data[1] = 0;
        }
        break;

    case 5: /* hold image 1 before fading to white */
        SetGpuReg(REG_OFFSET_BG0HOFS, 0);

        if (++data[1] >= 30)
        {
            BeginNormalPaletteFade(
                PALETTES_ALL,
                0,
                0,
                16,
                RGB_WHITEALPHA
            );

            data[0] = 6;
            data[1] = 0;
        }
        break;

    case 6: // hand off to the title screen
        if (!gPaletteFade.active)
        {
            SetMainCallback2(MainCB2_EndIntro);
            DestroyTask(taskId);
        }
        break;
    }
}

#define sPalIdx data[1]
#define sDelay data[2]

static void SpriteCB_Lightning(struct Sprite *sprite)
{
if (sprite->animEnded)
sprite->invisible = TRUE;

switch (sprite->sState)
 {
 case 0:
 sprite->sPalIdx = 0x1C2;
 sprite->sState++;
 case 1:
 if (!sTest3MountainLightningMode)
     CpuCopy16(INTRO3_RAW_PTR(sprite->sPalIdx), &gPlttBufferFaded[BG_PLTT_ID(5) + 13], PLTT_SIZEOF(1));
 sprite->sPalIdx += 2;
 if (sprite->sPalIdx != 0x1CE)
 break;
 sprite->sPalIdx = 0x1CC;
 sprite->sDelay = 4;
 sprite->sState++;
 case 2:
 if (--sprite->sDelay == 0)
 {
 sprite->sDelay = 4;
 if (!sTest3MountainLightningMode)
     CpuCopy16(INTRO3_RAW_PTR(sprite->sPalIdx), &gPlttBufferFaded[BG_PLTT_ID(5) + 13], PLTT_SIZEOF(1));
 sprite->sPalIdx -= 2;
 if (sprite->sPalIdx == 0x1C0)
 DestroySprite(sprite);
 }
 break;
 }

}

#undef sPalIdx
#undef sDelay

#define tRayquazaTaskId data[4]

static void Task_Scene3_LoadRayquazaAttack(u8 taskId)
{
u8 attackTaskId;

LoadCompressedSpriteSheet(sSpriteSheet_RayquazaOrb);
 LoadSpritePalettes(sSpritePalette_RayquazaOrb);
 SetGpuReg(REG_OFFSET_DISPCNT, DISPCNT_MODE_0
 | DISPCNT_OBJ_1D_MAP
 | DISPCNT_BG0_ON
 | DISPCNT_BG2_ON
 | DISPCNT_OBJ_ON
 | DISPCNT_WIN0_ON);
 gTasks[taskId].func = Task_Scene3_Rayquaza;
 BeginNormalPaletteFade(PALETTES_BG & ~(0x21), 0, 16, 0, RGB(9, 10, 10));
 gTasks[taskId].tState = 0;
 gTasks[taskId].data[1] = 0xA8;
 gTasks[taskId].data[2] = -0x10;
 gTasks[taskId].data[3] = -0x88;
 gTasks[taskId].data[4] = -0x10;
 attackTaskId = CreateTask(Task_RayquazaAttack, 0);
 gTasks[attackTaskId].tRayquazaTaskId = taskId;

}

static void Task_Scene3_Rayquaza(u8 taskId)
{
s16 *data = gTasks[taskId].data;

if (tTimer % 2 == 0)
 data[6] ^= 2;

tTimer++;

switch (tState)
 {
 case 0:
 if ((tTimer & 1) != 0)
 {
 data[1] -= 2;
 data[2];
 data[3] += 2;
 data[4];
 }
 if (data[1] == 0x68)
 {
 tState++;
 data[5] = 1;
 }
 break;
 case 1:
 tState++;
 data[5] = 4;
 break;
 case 2:
 data[1] += 4;
 data[2] -= 2;
 data[3] -= 4;
 data[4] -= 2;
 if (!gPaletteFade.active)
 {
 data[5] = 0x8C;
 tState++;
 }
 break;
 case 3:
 if (--data[5] == 0)
 gTasks[taskId].func = Task_EndIntroMovie;
 break;
 }

}
#undef tDelay
#undef tTimer

static void Task_EndIntroMovie(u8 taskId)
{
DestroyTask(taskId);
SetMainCallback2(MainCB2_EndIntro);
}

static void Task_RayquazaAttack(u8 taskId)
{
u8 spriteId;
s16 *data = gTasks[taskId].data;
data[2]++;

switch (tState)
 {
 case 0:
 if ((data[2] & 1) != 0)
 {
 CpuCopy16(INTRO3_RAW_PTR(0x1A2 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 14], PLTT_SIZEOF(1));
 data[1];
 }
 if (data[1] == 6)
 {
 tState;
 data[1] = 0;
 data[3] = 10;
 }
 break;
 case 1:
 if (data[3] == 0)
 {
 if ((data[2] & 1) != 0)
 {
 CpuCopy16(INTRO3_RAW_PTR(0x1A2 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 8], PLTT_SIZEOF(1));
 data[1];
 }
 if (data[1] == 6)
 {
 tState;
 data[3] = 10;
 }
 }
 else
 {
 data[3]--;
 }
 break;
 case 2:
 if (data[3] == 0)
 {
 if ((data[2] & 1) != 0)
 {
 CpuCopy16(INTRO3_RAW_PTR(0x182 + data[1] * 2), &gPlttBufferFaded[BG_PLTT_ID(5) + 12], PLTT_SIZEOF(1));
 data[1];
 }
 if (data[1] == 6)
 {
 spriteId = CreateSprite(&sSpriteTemplate_RayquazaOrb, 120, 88, 15);
 PlaySE(SE_INTRO_BLAST);
 gSprites[spriteId].invisible = TRUE;
 gSprites[spriteId].data[3] = tRayquazaTaskId;
 tState;
 data[3] = 16;
 }
 }
 else
 {
 data[3]--;
 }
 break;
 case 3:
 if ((data[2] & 1) != 0)
 {
 if (--data[3] != 0)
 {
 BlendPalette(BG_PLTT_ID(5), 16, data[3], RGB(9, 10, 10));
 CpuCopy16(INTRO3_RAW_PTR(428), &gPlttBufferFaded[BG_PLTT_ID(5) + 14], PLTT_SIZEOF(1));
 CpuCopy16(INTRO3_RAW_PTR(428), &gPlttBufferFaded[BG_PLTT_ID(5) + 8], PLTT_SIZEOF(1));
 CpuCopy16(INTRO3_RAW_PTR(396), &gPlttBufferFaded[BG_PLTT_ID(5) + 12], PLTT_SIZEOF(1));
 }
 else
 {
 tState++;
 data[3] = 53;
 }
 }
 break;
 case 4:
 if (--data[3] == 0)
 {
 BeginNormalPaletteFade(PALETTES_ALL, 0, 0, 16, RGB_WHITE);
 tState++;
 }
 break;
 case 5:
 if (!gPaletteFade.active)
 DestroyTask(taskId);
 break;
 }

}

static void PrepareIntroSceneTransition(void)
{
    /*
     * Stop scanline/HBlank effects before changing VRAM or display
     * registers. Otherwise the old scene can write display registers
     * during the first frame of the new scene.
     */
    SetHBlankCallback(NULL);
    ScanlineEffect_Stop();
    ScanlineEffect_Clear();

    /*
     * Stop the old scene's VBlank work before replacing its graphics,
     * palettes, and sprites.
     */
    SetVBlankCallback(NULL);

    /*
     * Hide the display before any bulk VRAM/OAM/palette operation.
     */
    SetGpuReg(REG_OFFSET_DISPCNT, 0);

    SetGpuReg(REG_OFFSET_BLDCNT, 0);
    SetGpuReg(REG_OFFSET_BLDALPHA, 0);
    SetGpuReg(REG_OFFSET_BLDY, 0);

    SetGpuReg(REG_OFFSET_BG0HOFS, 0);
    SetGpuReg(REG_OFFSET_BG0VOFS, 0);
    SetGpuReg(REG_OFFSET_BG1HOFS, 0);
    SetGpuReg(REG_OFFSET_BG1VOFS, 0);
    SetGpuReg(REG_OFFSET_BG2HOFS, 0);
    SetGpuReg(REG_OFFSET_BG2VOFS, 0);
    SetGpuReg(REG_OFFSET_BG3HOFS, 0);
    SetGpuReg(REG_OFFSET_BG3VOFS, 0);

    SetGpuReg(REG_OFFSET_BG2PA, 0x100);
    SetGpuReg(REG_OFFSET_BG2PB, 0);
    SetGpuReg(REG_OFFSET_BG2PC, 0);
    SetGpuReg(REG_OFFSET_BG2PD, 0x100);
    SetGpuReg(REG_OFFSET_BG2X_L, 0);
    SetGpuReg(REG_OFFSET_BG2X_H, 0);
    SetGpuReg(REG_OFFSET_BG2Y_L, 0);
    SetGpuReg(REG_OFFSET_BG2Y_H, 0);
}

static void IntroResetGpuRegs(void)
{
SetGpuReg(REG_OFFSET_DISPCNT, 0);
SetGpuReg(REG_OFFSET_BG3HOFS, 0);
SetGpuReg(REG_OFFSET_BG3VOFS, 0);
SetGpuReg(REG_OFFSET_BG2HOFS, 0);
SetGpuReg(REG_OFFSET_BG2VOFS, 0);
SetGpuReg(REG_OFFSET_BG1HOFS, 0);
SetGpuReg(REG_OFFSET_BG1VOFS, 0);
SetGpuReg(REG_OFFSET_BG0HOFS, 0);
SetGpuReg(REG_OFFSET_BG0VOFS, 0);
SetGpuReg(REG_OFFSET_BLDCNT, 0);
SetGpuReg(REG_OFFSET_BLDALPHA, 0);
SetGpuReg(REG_OFFSET_BLDY, 0);
}

static void Task_BlendLogoIn(u8 taskId)
{
switch (gTasks[taskId].tState)
{
case 0:
default:
SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND
| BLDCNT_TGT2_BG0
| BLDCNT_TGT2_BG1
| BLDCNT_TGT2_BG2
| BLDCNT_TGT2_BG3
| BLDCNT_TGT2_OBJ
| BLDCNT_TGT2_BD);
SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[31]);
SetGpuReg(REG_OFFSET_BLDY, 0);
gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend);
gTasks[taskId].tState++;
break;
case 1:
if (gTasks[taskId].data[1] != 0)
{
u8 tmp;

gTasks[taskId].data[1]--;
 tmp = gTasks[taskId].data[1] / 2;
 SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[tmp]);
 }
 else
 {
 SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[0]);
 gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend) / 4;
 gTasks[taskId].tState++;
 }
 break;
 case 2:
 SetGpuReg(REG_OFFSET_BLDCNT, 0);
 SetGpuReg(REG_OFFSET_BLDALPHA, 0);
 SetGpuReg(REG_OFFSET_BLDY, 0);
 DestroyTask(taskId);
 break;
 }

}

static void Task_BlendLogoOut(u8 taskId)
{
switch (gTasks[taskId].tState)
{
case 0:
default:
SetGpuReg(REG_OFFSET_BLDCNT, BLDCNT_EFFECT_BLEND
| BLDCNT_TGT2_BG0
| BLDCNT_TGT2_BG1
| BLDCNT_TGT2_BG2
| BLDCNT_TGT2_BG3
| BLDCNT_TGT2_OBJ
| BLDCNT_TGT2_BD);
SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[0]);
SetGpuReg(REG_OFFSET_BLDY, 0);
gTasks[taskId].data[1] = 0;
gTasks[taskId].tState++;
break;
case 1:
if (gTasks[taskId].data[1] < (int)ARRAY_COUNT(gTitleScreenAlphaBlend) - 2)
{
u8 tmp;

gTasks[taskId].data[1];
 tmp = gTasks[taskId].data[1] / 2;
 SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[tmp]);
 }
 else
 {
 SetGpuReg(REG_OFFSET_BLDALPHA, gTitleScreenAlphaBlend[31]);
 gTasks[taskId].data[1] = ARRAY_COUNT(gTitleScreenAlphaBlend) / 4;
 gTasks[taskId].tState;
 }
 break;
 case 2:
 if (gTasks[taskId].data[1] != 0)
 {
 gTasks[taskId].data[1]--;
 }
 else
 {
 SetGpuReg(REG_OFFSET_BLDCNT, 0);
 SetGpuReg(REG_OFFSET_BLDALPHA, 0);
 SetGpuReg(REG_OFFSET_BLDY, 0);
 DestroyTask(taskId);
 }
 break;
 }

}

static void PanAndScaleWater(u16 screenX,
u16 screenY,
u16 scaleX,
u16 scaleY,
u16 angle)
{
struct BgAffineSrcData src;
struct BgAffineDstData dest;

src.texX = 0x8000;
 src.texY = 0;
 src.scrX = screenX;
 src.scrY = screenY;
 src.sx = scaleX;
 src.sy = scaleY;
 src.alpha = angle;

BgAffineSet(&src, &dest, 1);

SetGpuReg(REG_OFFSET_BG2PA, dest.pa);
 SetGpuReg(REG_OFFSET_BG2PB, dest.pb);
 SetGpuReg(REG_OFFSET_BG2PC, dest.pc);
 SetGpuReg(REG_OFFSET_BG2PD, dest.pd);
 SetGpuReg(REG_OFFSET_BG2X_L, dest.dx);
 SetGpuReg(REG_OFFSET_BG2X_H, dest.dx >> 16);
 SetGpuReg(REG_OFFSET_BG2Y_L, dest.dy);
 SetGpuReg(REG_OFFSET_BG2Y_H, dest.dy >> 16);

}

void PanFadeAndZoomScreen(u16 screenX, u16 screenY, u16 zoom, u16 alpha)
{
struct BgAffineSrcData src;
struct BgAffineDstData dest;

src.texX = 0x8000;
 src.texY = 0x8000;
 src.scrX = screenX;
 src.scrY = screenY;
 src.sx = zoom;
 src.sy = zoom;
 src.alpha = alpha;
 BgAffineSet(&src, &dest, 1);
 SetGpuReg(REG_OFFSET_BG2PA, dest.pa);
 SetGpuReg(REG_OFFSET_BG2PB, dest.pb);
 SetGpuReg(REG_OFFSET_BG2PC, dest.pc);
 SetGpuReg(REG_OFFSET_BG2PD, dest.pd);
 SetGpuReg(REG_OFFSET_BG2X_L, dest.dx);
 SetGpuReg(REG_OFFSET_BG2X_H, dest.dx >> 16);
 SetGpuReg(REG_OFFSET_BG2Y_L, dest.dy);
 SetGpuReg(REG_OFFSET_BG2Y_H, dest.dy >> 16);

}

static void SpriteCB_WaterDrop_Ripple(struct Sprite *sprite)
{
u8 palNum;

if (sprite->data[2] >= 192)
 {
 if (sprite->data[3] != 0)
 {
 sprite->data[3]--;
 }
 else
 {
 sprite->invisible = FALSE;
 SetOamMatrix(sprite->data[1], sprite->data[2], 0, 0, sprite->data[2]);
 sprite->data[2] = (sprite->data[2] * 95) / 100;
 palNum = (sprite->data[2] - 192) / 128 + 9;
 if (palNum > 15)
 palNum = 15;
 sprite->oam.paletteNum = palNum;
 }
 }
 else
 {
 DestroySprite(sprite);
 }

}

static void SpriteCB_WaterDropHalf(struct Sprite *sprite)
{
if (gSprites[sprite->data[7]].data[7] != 0)
{
sprite->invisible = TRUE;
sprite->x += sprite->x2;
sprite->y += sprite->y2;
StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
sprite->data[2] = 1024;
sprite->data[3] = 8 * (sprite->data[1] & 3);
sprite->callback = SpriteCB_WaterDrop_Ripple;
sprite->oam.shape = SPRITE_SHAPE(64x32);
sprite->oam.size = SPRITE_SIZE(64x32);
CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
}
else
{
sprite->x2 = gSprites[sprite->data[7]].x2;
sprite->y2 = gSprites[sprite->data[7]].y2;
sprite->x = gSprites[sprite->data[7]].x;
sprite->y = gSprites[sprite->data[7]].y;
}
}

static void SpriteCB_WaterDrop(struct Sprite *sprite)
{
// Wait for sState to be modified by Task_Scene1_WaterDrops
if (sprite->sState != 0)
sprite->callback = SpriteCB_WaterDrop_Slide;
}

static void SpriteCB_WaterDrop_Slide(struct Sprite *sprite)
{
if (sprite->x <= 116)
{
sprite->y += sprite->y2;
sprite->y2 = 0;
sprite->x += 4;
sprite->x2 = -4;
sprite->data[4] = 128;
sprite->callback = SpriteCB_WaterDrop_ReachLeafEnd;
}
else
{
u16 data2;
u16 data3;
u16 data4;
s16 sin1;
s16 sin2;
s16 sin3;
s16 sin4;
s16 var1;
s16 var2;
s16 var3;
s16 var4;
s16 temp;

data4 = sprite->data[4];
 sin1 = gSineTable[(u8)data4];
 sin2 = gSineTable[(u8)(data4 + 64)];
 sprite->data[4] += 2;
 sprite->y2 = sin1 / 32;
 sprite->x--;
 if (sprite->x & 1)
 sprite->y++;
 temp = -sin2 / 16;
 data2 = sprite->data[2];
 data3 = sprite->data[3];
 sin3 = gSineTable[(u8)(temp - 16)];
 sin4 = gSineTable[(u8)(temp + 48)];
 var1 = sin4 * data2 / 256;
 var2 = -sin3 * data3 / 256;
 var3 = sin3 * data2 / 256;
 var4 = sin4 * data3 / 256;
 SetOamMatrix(sprite->data[1], data2, 0, 0, data3);
 SetOamMatrix(sprite->data[1] + 1, var1, var3, var2, var4);
 SetOamMatrix(sprite->data[1] + 2, var1, var3, var2 * 2, var4 * 2);
 }

}

static void SpriteCB_WaterDrop_ReachLeafEnd(struct Sprite *sprite)
{
SetOamMatrix(sprite->data[1], sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
SetOamMatrix(sprite->data[1] + 1, sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
SetOamMatrix(sprite->data[1] + 2, sprite->data[6] + 64, 0, 0, sprite->data[6] + 64);
if (sprite->data[4] != MAX_SPRITES)
{
u16 sinIdx;
sprite->data[4] -= 8;
sinIdx = sprite->data[4];
sprite->x2 = gSineTable[(u8)(sinIdx + 64)] / 64;
sprite->y2 = gSineTable[(u8)sinIdx] / 64;
}
else
{
sprite->data[4] = 0;
sprite->callback = SpriteCB_WaterDrop_DangleFromLeaf;
}
}

static void SpriteCB_WaterDrop_DangleFromLeaf(struct Sprite *sprite)
{
if (sprite->data[0] != 2)
{
s16 r2;

sprite->data[4] += 8;
 r2 = gSineTable[(u8)sprite->data[4]] / 16 + 64;
 sprite->x2 = gSineTable[(u8)(r2 + 64)] / 64;
 sprite->y2 = gSineTable[(u8)r2] / 64;
 }
 else
 {
 sprite->callback = SpriteCB_WaterDrop_Fall;
 }

}

static void SpriteCB_WaterDrop_Fall(struct Sprite *sprite)
{
if (sprite->y < sprite->data[5])
{
sprite->y += 4;
}
else
{
sprite->data[7] = 1;
sprite->invisible = TRUE;
sprite->x += sprite->x2;
sprite->y += sprite->y2;
StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
sprite->data[2] = 1024;
sprite->data[3] = 8 * (sprite->data[1] & 3);
sprite->callback = SpriteCB_WaterDrop_Ripple;
sprite->oam.shape = SPRITE_SHAPE(64x32);
sprite->oam.size = SPRITE_SIZE(64x32);
CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
}
}

// Identical to SpriteCB_WaterDrop_Fall
// Used by the 2nd and 3rd water drops to skip the leaf slide
static void SpriteCB_WaterDropShort(struct Sprite *sprite)
{
if (sprite->y < sprite->data[5])
{
sprite->y += 4;
}
else
{
sprite->data[7] = 1;
sprite->invisible = TRUE;
sprite->x += sprite->x2;
sprite->y += sprite->y2;
StartSpriteAnim(sprite, DROP_ANIM_RIPPLE);
sprite->data[2] = 1024;
sprite->data[3] = 8 * (sprite->data[1] & 3);
sprite->callback = SpriteCB_WaterDrop_Ripple;
sprite->oam.shape = SPRITE_SHAPE(64x32);
sprite->oam.size = SPRITE_SIZE(64x32);
CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_ERASE);
}
}

static u8 CreateWaterDrop(s16 x, s16 y, u16 c, u16 d, u16 e, u8 fallImmediately)
{
u8 spriteId;
u8 oldSpriteId;

// Create water drop reflection
 spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
 gSprites[spriteId].data[0] = 0;
 gSprites[spriteId].data[7] = 0;
 gSprites[spriteId].data[1] = d;
 gSprites[spriteId].data[2] = c;
 gSprites[spriteId].data[3] = c;
 gSprites[spriteId].data[5] = e;
 gSprites[spriteId].data[6] = c;
 gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
 gSprites[spriteId].oam.matrixNum = d;
 CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
 StartSpriteAnim(&gSprites[spriteId], DROP_ANIM_REFLECTION);
 if (!fallImmediately)
 gSprites[spriteId].callback = SpriteCB_WaterDrop; // Do full anim, for 1st drop that slides along the leaf
 else
 gSprites[spriteId].callback = SpriteCB_WaterDropShort; // Skip to drop falling into the water, for 2nd and 3rd drops
 oldSpriteId = spriteId;

// Create water drop upper half
 // Implicitly anim number 0, DROP_ANIM_UPPER_HALF
 spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
 gSprites[spriteId].data[7] = oldSpriteId;
 gSprites[spriteId].data[1] = d + 1;
 gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
 gSprites[spriteId].oam.matrixNum = d + 1;
 CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
 gSprites[spriteId].callback = SpriteCB_WaterDropHalf;

// Create water drop lower half
 spriteId = CreateSprite(&sSpriteTemplate_WaterDrop, x, y, 1);
 gSprites[spriteId].data[7] = oldSpriteId;
 gSprites[spriteId].data[1] = d + 2;
 StartSpriteAnim(&gSprites[spriteId], DROP_ANIM_LOWER_HALF);
 gSprites[spriteId].oam.affineMode = ST_OAM_AFFINE_DOUBLE;
 gSprites[spriteId].oam.matrixNum = d + 2;
 CalcCenterToCornerVec(&gSprites[spriteId], SPRITE_SHAPE(32x32), SPRITE_SIZE(32x32), ST_OAM_AFFINE_ERASE);
 gSprites[spriteId].callback = SpriteCB_WaterDropHalf;

SetOamMatrix(d, c + 32, 0, 0, c + 32);
 SetOamMatrix(d + 1, c + 32, 0, 0, c + 32);
 SetOamMatrix(d + 2, c + 32, 0, 0, 2 * (c + 32));

return oldSpriteId;

}

// State is handled by Task_Scene2_BikeRide
static void SpriteCB_PlayerOnBicycle(struct Sprite *sprite)
{
// Adjust x position
switch (sprite->sState)
{
case 0:
// Move forwards
StartSpriteAnimIfDifferent(sprite, 0);
sprite->x--;
break;
case 1:
// Drift backwards slowly
StartSpriteAnimIfDifferent(sprite, 0);
if (gIntroFrameCounter & 7)
return;
sprite->x++;
break;
case 2:
// Move backwards
if (sprite->x <= 120 || gIntroFrameCounter & 7)
sprite->x++;
break;
case 3:
// Bike in place
break;
case 4:
// Exit to the left
if (sprite->x > -32)
sprite->x -= 2;
break;
}

if (gIntroFrameCounter & 7)
 return;

// Adjust y position
 if (sprite->y2 != 0)
 {
 // Return to neutral after wobble
 sprite->y2 = 0;
 }
 else
 {
 // Random wobble on y axis
 switch (Random() & 3)
 {
 case 0:
 sprite->y2 = -1;
 break;
 case 1:
 sprite->y2 = 1;
 break;
 case 2:
 case 3:
 sprite->y2 = 0;
 break;
 }
 }

}

#define sSinIdx data[1]

// Movement is started by setting state to 1 in Task_Scene2_BikeRide
static void SpriteCB_Flygon(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
break;
case 1:
if (sprite->x2 + sprite->x < DISPLAY_WIDTH + 64)
sprite->x2 += 8;
else
sprite->sState = 2;
break;
case 2:
if (sprite->x2 + sprite->x > 120)
sprite->x2 -= 1;
else
sprite->sState = 3;
break;
case 3:
if (sprite->x2 > 0)
sprite->x2 -= 2;
break;
}
sprite->y2 = Sin((u8)sprite->sSinIdx, 8) - sFlygonYOffset;
sprite->sSinIdx += 4;
}

#undef sSinIdx

#define sTimer data[1]
#define sLetterId data[2]
#define sColorDelay data[3]
#define sLetterX data[3] // Re-used

// For the letters in "Game Freak"
// Also intended for the letters in "Presents", which is never shown
static void SpriteCB_LogoLetter(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
if (sprite->sTimer != 0)
{
sprite->sTimer--;
}
else
{
// Start the grow and shrink appearance animation
sprite->invisible = FALSE;
StartSpriteAffineAnim(sprite, 1);
sprite->sState++;
}
break;
case 1:
if (gIntroFrameCounter == TIMER_LOGO_LETTERS_COLOR)
{
// Skip blue color fade entirely to keep the original text color
sprite->sState = 4;
}
break;
case 4:
if (gIntroFrameCounter == TIMER_LOGO_DISAPPEAR)
{
// Start simple fade out (using object blend mode) instead of growing big
sprite->oam.objMode = ST_OAM_OBJ_BLEND;
sprite->sTimer = 16; // Fade duration frames
sprite->sState++;
}
break;
case 5:
// Fade out smoothly
if (sprite->sTimer != 0)
{
sprite->sTimer--;
}
else
{
DestroySprite(sprite);
}
break;
}
}

#define sDWorksTimer data[0]
#define sDWorksState data[1]

static void SpriteCB_DWorksLogo(struct Sprite *sprite)
{
    switch (sprite->sDWorksState)
    {
        case 0:
            // Wait for your desired frame to appear (e.g., frame 300)
            if (gIntroFrameCounter == 300)
            {
                sprite->invisible = FALSE;
                sprite->sDWorksState++;
            }
            break;
        case 1:
            // Hold on screen for roughly 2.5 seconds (150 frames)
            if (++sprite->sDWorksTimer > 150)
            {
                sprite->sDWorksTimer = 0;
                sprite->sDWorksState++;
            }
            break;
        case 2:
            DestroySprite(sprite);
            break;
    }
}

#undef sDWorksTimer
#undef sDWorksState

static void SpriteCB_GameFreakLogo(struct Sprite *sprite)
{
switch (sprite->sState)
{
case 0:
if (gIntroFrameCounter == TIMER_LOGO_APPEAR)
{
sprite->invisible = FALSE;
sprite->sState++;
}
break;
case 1:
if (gIntroFrameCounter == TIMER_LOGO_DISAPPEAR)
{
// Switch to blend mode for a clean fade out instead of scaling up
sprite->oam.objMode = ST_OAM_OBJ_BLEND;
sprite->sTimer = 16;
sprite->sState++;
}
break;
case 2:
if (sprite->sTimer != 0)
{
sprite->sTimer--;
}
else
{
DestroySprite(sprite);
}
break;
}
}

static u8 CreateGameFreakLogoSprites(s16 x, s16 y, s16 unused)
{
u16 i;
u8 spriteId;

// Create "Game Freak" letters
 for (i = 0; i < NUM_GF_LETTERS; i++)
 {
 spriteId = CreateSprite(&sSpriteTemplate_GameFreakLetter, sGameFreakLetterData[i][1] + x, y - 4, 0);
 gSprites[spriteId].sState = 0;
 gSprites[spriteId].sTimer = sGameFreakLetterStartDelays[i];
 gSprites[spriteId].sLetterId = i;
 gSprites[spriteId].invisible = TRUE;
 gSprites[spriteId].oam.matrixNum = i + 12;
 StartSpriteAnim(&gSprites[spriteId], sGameFreakLetterData[i][0]);
 StartSpriteAffineAnim(&gSprites[spriteId], 0);
 }

// Create Game Freak logo
 spriteId = CreateSprite(&sSpriteTemplate_GameFreakLogo, 120, y - 6, 0);
 gSprites[spriteId].sState = 0;
 gSprites[spriteId].invisible = TRUE;
 gSprites[spriteId].oam.matrixNum = i + 12;
 StartSpriteAffineAnim(&gSprites[spriteId], 1);

return spriteId;

}

#undef sTimer
#undef sLetterId
#undef sColorDelay
#undef sLetterX
#undef COLOR_CHANGES

#define sScale data[1]
#define sRot data[2]
#define sPos data[3]
#define sTimer data[7]

static void SpriteCB_FlygonSilhouette(struct Sprite *sprite)
{
sprite->sTimer++;

if (sprite->sState != 0)
 {
 s16 sin;
 s16 cos;

s16 a, b, c, d;
 // Determines rotation of the sprite
 sin = gSineTable[(u8)sprite->sRot];
 cos = gSineTable[(u8)(sprite->sRot + 64)];
 // Converts rotation and scale into the OAM matrix
 d = Q_8_8_TO_INT( cos * sprite->sScale);
 c = Q_8_8_TO_INT(-sin * sprite->sScale);
 b = Q_8_8_TO_INT( sin * sprite->sScale);
 a = Q_8_8_TO_INT( cos * sprite->sScale);

SetOamMatrix(1, a, b, c, d);
 }

switch (sprite->sState)
 {
 case 0:
 default:
 sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
 sprite->oam.matrixNum = 1;
 CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x32), SPRITE_SIZE(64x32), ST_OAM_AFFINE_DOUBLE);
 sprite->invisible = FALSE;
 sprite->sState = 1;
 sprite->sScale = 128;
 sprite->sRot = 0;
 sprite->sPos = 0;
 break;
 case 1:
 sprite->x2 = -Sin((u8)sprite->sPos, 140);
 sprite->y2 = -Sin((u8)sprite->sPos, 120);
 sprite->sScale += 7;
 sprite->sPos += 3;
 if (sprite->x + sprite->x2 <= -16)
 {
 sprite->oam.priority = 3;
 sprite->sState++;
 sprite->x = 20;
 sprite->y = 40;
 sprite->sScale = 512;
 sprite->sRot = 0;
 sprite->sPos = 16;
 }
 break;
 case 2:
 sprite->x2 = Sin((u8)sprite->sPos, 34);
 sprite->y2 = -Cos((u8)sprite->sPos, 60);
 sprite->sScale += 2;
 if (sprite->sTimer % 5 == 0)
 sprite->sPos++;
 break;
 }

}

#undef sScale
#undef sRot
#undef sPos
#undef sTimer

static void SpriteCB_RayquazaOrb(struct Sprite *sprite)
{
u16 foo;
switch (sprite->sState)
{
case 0:
default:
sprite->invisible = FALSE;
sprite->oam.affineMode = ST_OAM_AFFINE_DOUBLE;
sprite->oam.matrixNum = 18;
CalcCenterToCornerVec(sprite, SPRITE_SHAPE(64x64), SPRITE_SIZE(64x64), ST_OAM_AFFINE_DOUBLE);
sprite->data[1] = 0;
sprite->sState = 1;
//fall through
case 1:
sprite->data[7];
if (sprite->data[7] & 1)
{
sprite->invisible = TRUE;
}
else
{
sprite->invisible = FALSE;
if (sprite->data[1] < 64)
sprite->data[1];
}
foo = 256 - gSineTable[(u8)sprite->data[1]] / 2;
SetOamMatrix(18, foo, 0, 0, foo);
break;
}
}
