/**************************************************************************
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   CX16-BATTLEBLOCKS is free software: you can redistribute it and/or   *
 *   modify it under the terms of the GNU General Public License as       *
 *   published by the Free Software Foundation, either version 3 of the   *
 *   License, or (at your option) any later version.                      *
 *                                                                        *
 *   CX16-BATTLEBLOCKS is distributed in the hope that it will be         *
 *   useful, but WITHOUT ANY WARRANTY; without even the implied           *
 *   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.     *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program. If not, see http://www.gnu.org/licenses/.   *
 *                                                                        *
 *************************************************************************/

#include "pieces.h"

/*
 * Tetromino geometry and preview tiles, ordered I, O, T, S, Z, J, L.
 * Coordinate pairs are x/y; rotation tables are indexed by type and rotation.
 */

/**
 * @brief Spawn offsets indexed by type, cell and x/y coordinate
 */
const int8_t pieces_start_table[7][4][2] = {
    {{0,0}, {1,0}, {2,0}, {3,0}},        // I
    {{1,-1}, {2,-1}, {1,0}, {2,0}},      // O
    {{2,0}, {1,0}, {3,0}, {2,-1}},       // T
    {{1,0}, {2,0}, {2,-1}, {3,-1}},      // S
    {{1,-1}, {2,-1}, {2,0}, {3,0}},      // Z
    {{1,-1}, {1,0}, {2,0}, {3,0}},       // J
    {{1,0}, {2,0}, {3,0}, {3,-1}},       // L
};

/**
 * @brief Rotation deltas indexed by type, rotation, cell and x/y
 */
const int8_t piece_rotation_table[7][4][4][2] = {
    // I
    {
        {{2,-1}, {1,0}, {0,1}, {-1,2}},
        {{1,2}, {0,1}, {-1,0}, {-2,-1}},
        {{-2,1}, {-1,0}, {0,-1}, {1,-2}},
        {{-1,-2}, {0,-1}, {1,0}, {2,1}}
    },
    // O
    {
        {{0,0}, {0,0}, {0,0}, {0,0}},
        {{0,0}, {0,0}, {0,0}, {0,0}},
        {{0,0}, {0,0}, {0,0}, {0,0}},
        {{0,0}, {0,0}, {0,0}, {0,0}}
    },
    // T
    {
        {{0,0}, {1,-1}, {-1,1}, {1,1}},
        {{0,0}, {1,1}, {-1,-1}, {-1,1}},
        {{0,0}, {-1,1}, {1,-1}, {-1,-1}},
        {{0,0}, {-1,-1}, {1,1}, {1,-1}}
    },
    // S
    {
        {{1,-1}, {0,0}, {1,1}, {0,2}},
        {{1,1}, {0,0}, {-1,1}, {-2,0}},
        {{-1,1}, {0,0}, {-1,-1}, {0,-2}},
        {{-1,-1}, {0,0}, {1,-1}, {2,0}}
    },
    // Z
    {
        {{2,0}, {1,1}, {0,0}, {-1,1}},
        {{0,2}, {-1,1}, {0,0}, {-1,-1}},
        {{-2,0}, {-1,-1}, {0,0}, {1,-1}},
        {{0,-2}, {1,-1}, {0,0}, {1,1}}
    },
    // J
    {
        {{2,0}, {1,-1}, {0,0}, {-1,1}},
        {{0,2}, {1,1}, {0,0}, {-1,-1}},
        {{-2,0}, {-1,1}, {0,0}, {1,-1}},
        {{0,-2}, {-1,-1}, {0,0}, {1,1}}
    },
    // L
    {
        {{1,-1}, {0,0}, {-1,1}, {0,2}},
        {{1,1}, {0,0}, {-1,-1}, {-2,0}},
        {{-1,1}, {0,0}, {1,-1}, {0,-2}},
        {{-1,-1}, {0,0}, {1,1}, {2,0}}
    }
};

/* Normalized occupancy for piece_rotation_table. The emulator AI tests
 * verify these tables against the rotation deltas for every tetromino.
 */

/**
 * @brief Bottom-up occupancy with zero-based x/y bounds for each rotation
 */
const uint8_t pieces_normalized[7][4][4][2] = {
    {{{0,0}, {1,0}, {2,0}, {3,0}}, {{0,3}, {0,2}, {0,1}, {0,0}}, {{3,0}, {2,0}, {1,0}, {0,0}}, {{0,0}, {0,1}, {0,2}, {0,3}}},
    {{{0,1}, {1,1}, {0,0}, {1,0}}, {{0,1}, {1,1}, {0,0}, {1,0}}, {{0,1}, {1,1}, {0,0}, {1,0}}, {{0,1}, {1,1}, {0,0}, {1,0}}},
    {{{1,0}, {0,0}, {2,0}, {1,1}}, {{0,1}, {0,2}, {0,0}, {1,1}}, {{1,1}, {2,1}, {0,1}, {1,0}}, {{1,1}, {1,0}, {1,2}, {0,1}}},
    {{{0,0}, {1,0}, {1,1}, {2,1}}, {{0,2}, {0,1}, {1,1}, {1,0}}, {{2,1}, {1,1}, {1,0}, {0,0}}, {{1,0}, {1,1}, {0,1}, {0,2}}},
    {{{0,1}, {1,1}, {1,0}, {2,0}}, {{1,2}, {1,1}, {0,1}, {0,0}}, {{2,0}, {1,0}, {1,1}, {0,1}}, {{0,0}, {0,1}, {1,1}, {1,2}}},
    {{{0,1}, {0,0}, {1,0}, {2,0}}, {{1,2}, {0,2}, {0,1}, {0,0}}, {{2,0}, {2,1}, {1,1}, {0,1}}, {{0,0}, {1,0}, {1,1}, {1,2}}},
    {{{0,0}, {1,0}, {2,0}, {2,1}}, {{0,2}, {0,1}, {0,0}, {1,0}}, {{2,1}, {1,1}, {0,1}, {0,0}}, {{1,0}, {1,1}, {1,2}, {0,2}}},
};

/**
 * @brief Occupied width in board cells for each type and rotation
 */
const uint8_t pieces_widths[7][4] = {
    {4, 1, 4, 1},
    {2, 2, 2, 2},
    {3, 2, 3, 2},
    {3, 2, 3, 2},
    {3, 2, 3, 2},
    {3, 2, 3, 2},
    {3, 2, 3, 2},
};

/**
 * @brief Bit mask selecting geometrically distinct rotations for each type
 */
const uint8_t pieces_unique_rotations[7] = {0x03, 0x01, 0x0F, 0x03, 0x03, 0x0F, 0x0F};

/**
 * @brief Twelve tile IDs per piece, laid out as a four-by-three preview
 */
extern const uint8_t piece_next_drawing_table[7][12] = {
    // I
    {0, 0, 0, 0,
     16, 16, 16, 16,
     0, 0, 0, 0},
    // O
    {0, 95, 95, 0,
     0, 93, 93, 0,
     0, 94, 94, 0},
    // T
    {0, 92, 84, 0,
     85, 86, 87, 88,
     89, 90, 90, 91},
    // S
    {0, 73, 64, 65,
     66, 67, 68, 69,
     70, 71, 72, 0},
    // Z
    {83, 74, 75, 0,
     76, 77, 78, 79,
     0, 80, 81, 82},
    // J
    {63, 56, 0, 0,
     57, 58, 59, 56,
     60, 61, 61, 62},
    // L
    {0, 0, 48, 55,
     48, 50, 52, 53,
     49, 51, 51, 54},
};
