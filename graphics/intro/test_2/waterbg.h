
//{{BLOCK(waterbg)

//======================================================================
//
//	waterbg, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 331 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 10592 + 2048 = 13152
//
//	Time-stamp: 2026-10-29, 19:47:50
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_WATERBG_H
#define GRIT_WATERBG_H

#define waterbgTilesLen 10592
extern const unsigned int waterbgTiles[2648];

#define waterbgMapLen 2048
extern const unsigned short waterbgMap[1024];

#define waterbgPalLen 512
extern const unsigned short waterbgPal[256];

#endif // GRIT_WATERBG_H

//}}BLOCK(waterbg)
