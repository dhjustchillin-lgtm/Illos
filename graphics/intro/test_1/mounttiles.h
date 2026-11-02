
//{{BLOCK(mounttiles)

//======================================================================
//
//	mounttiles, 256x256@4, 
//	Transparent color : FF,00,FF
//	+ palette 256 entries, not compressed
//	+ 143 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 4576 + 2048 = 7136
//
//	Time-stamp: 2026-10-24, 21:26:11
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_MOUNTTILES_H
#define GRIT_MOUNTTILES_H

#define mounttilesTilesLen 4576
extern const unsigned int mounttilesTiles[1144];

#define mounttilesMapLen 2048
extern const unsigned short mounttilesMap[1024];

#define mounttilesPalLen 512
extern const unsigned short mounttilesPal[256];

#endif // GRIT_MOUNTTILES_H

//}}BLOCK(mounttiles)
