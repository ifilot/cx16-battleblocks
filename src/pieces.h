#ifndef _PIECES_H
#define _PIECES_H

#include <stdint.h>

extern const int8_t pieces_start_table[7][4][2];

// order: type, rotation id, piece idx, coordinate
extern const int8_t piece_rotation_table[7][4][4][2];

#endif // _PIECES_H