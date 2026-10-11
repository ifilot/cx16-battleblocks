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

#ifndef _PIECES_H
#define _PIECES_H

#include <stdint.h>

/*
 * Tetromino geometry and preview tiles, ordered I, O, T, S, Z, J, L.
 * Coordinate pairs are x/y; rotation tables are indexed by type and rotation.
 */

/**
 * @brief Spawn offsets indexed by type, cell and x/y coordinate
 */
extern const int8_t pieces_start_table[7][4][2];

/**
 * @brief Rotation deltas indexed by type, rotation, cell and x/y
 */
extern const int8_t piece_rotation_table[7][4][4][2];

/**
 * @brief Bottom-up occupancy with zero-based x/y bounds for each rotation
 */
extern const uint8_t pieces_normalized[7][4][4][2];

/**
 * @brief Occupied width in board cells for each type and rotation
 */
extern const uint8_t pieces_widths[7][4];

/**
 * @brief Bit mask selecting geometrically distinct rotations for each type
 */
extern const uint8_t pieces_unique_rotations[7];

/**
 * @brief Twelve tile IDs per piece, laid out as a four-by-three preview
 */
extern const uint8_t piece_next_drawing_table[7][12];

#endif // _PIECES_H
