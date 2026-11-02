
//{{BLOCK(grassbg)

//======================================================================
//
//	grassbg, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 374 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 11968 + 2048 = 14528
//
//	Time-stamp: 2026-10-29, 19:47:38
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_GRASSBG_H
#define GRIT_GRASSBG_H

#define grassbgTilesLen 11968
extern const unsigned int grassbgTiles[2992];

#define grassbgMapLen 2048
extern const unsigned short grassbgMap[1024];

#define grassbgPalLen 512
extern const unsigned short grassbgPal[256];

#endif // GRIT_GRASSBG_H

//}}BLOCK(grassbg)
