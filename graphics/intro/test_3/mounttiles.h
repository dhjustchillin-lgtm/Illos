
//{{BLOCK(mounttiles)

//======================================================================
//
//	mounttiles, 256x256@4, 
//	+ palette 256 entries, not compressed
//	+ 132 tiles (t|f|p reduced) not compressed
//	+ regular map (in SBBs), not compressed, 32x32 
//	Total size: 512 + 4224 + 2048 = 6784
//
//	Time-stamp: 2026-11-02, 14:02:39
//	Exported by Cearn's GBA Image Transmogrifier, v1.23.0-blocks
//	( http://www.coranac.com/projects/#grit )
//
//======================================================================

#ifndef GRIT_MOUNTTILES_H
#define GRIT_MOUNTTILES_H

#define mounttilesTilesLen 4224
extern const unsigned int mounttilesTiles[1056];

#define mounttilesMapLen 2048
extern const unsigned short mounttilesMap[1024];

#define mounttilesPalLen 512
extern const unsigned short mounttilesPal[256];

#endif // GRIT_MOUNTTILES_H

//}}BLOCK(mounttiles)
