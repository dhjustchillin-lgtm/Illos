
//{{BLOCK(water)

//======================================================================
//
//	water, 256x256@8, 
//	+ palette 256 entries, not compressed
//	+ 65 tiles (t reduced) not compressed
//	+ affine map, not compressed, 32x32 
//	Total size: 512 + 4160 + 1024 = 5696
//
//	Time-stamp: 2026-10-23, 17:05:37
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_WATER_H
#define GRIT_WATER_H

#define waterTilesLen 4160
extern const unsigned int waterTiles[1040];

#define waterMapLen 1024
extern const unsigned short waterMap[512];

#define waterPalLen 512
extern const unsigned short waterPal[256];

#endif // GRIT_WATER_H

//}}BLOCK(water)
