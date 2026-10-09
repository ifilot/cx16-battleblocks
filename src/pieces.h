#ifndef _PIECES_H
#define _PIECES_H

#include <stdint.h>

extern const int8_t pieces_start_table[7][4][2];

// order: type, rotation id, piece idx, coordinate
extern const int8_t piece_rotation_table[7][4][4][2];

extern const uint8_t pieces_normalized[7][4][4][2];
extern const uint8_t pieces_widths[7][4];
extern const uint8_t pieces_unique_rotations[7];

extern const uint8_t piece_next_drawing_table[7][12];

#endif // _PIECES_H