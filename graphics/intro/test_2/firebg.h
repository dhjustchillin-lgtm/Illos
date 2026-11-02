
//{{BLOCK(firebg)

//======================================================================
//
//	firebg, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 516 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 16512 + 2048 = 19072
//
//	Time-stamp: 2026-10-29, 19:47:28
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_FIREBG_H
#define GRIT_FIREBG_H

#define firebgTilesLen 16512
extern const unsigned int firebgTiles[4128];

#define firebgMapLen 2048
extern const unsigned short firebgMap[1024];

#define firebgPalLen 512
extern const unsigned short firebgPal[256];

#endif // GRIT_FIREBG_H

//}}BLOCK(firebg)
