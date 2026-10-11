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

#ifndef _GAMESTATE_H
#define _GAMESTATE_H

#include <stdint.h>

#include "pieces.h"
#include "bagofseven.h"
#include "constants.h"

/*
 * Per-player match state. Board row zero is the bottom; rows 0..19 are
 * visible and rows 20..23 hold the hidden spawn area. Empty cells are 0xFF.
 */

extern struct GameState gs_player2;              // Opponent; paused during solo play

struct GameState {
    uint8_t player;                              // PLAYER1 or PLAYER2
    uint8_t piece_old_position[4][2];            // Previous active cells, erased on redraw
    uint8_t piece_position[4][2];                // Current active cells, x/y in board coordinates
    uint8_t piece_old_shadow[4][2];              // Previous landing cells, erased on redraw
    uint8_t piece_shadow[4][2];                  // Current hard-drop destination, x/y
    struct PieceList piece_list;                 // Current and next shuffled bags
    uint8_t piece_type;                          // Tetromino type, 0..6
    uint8_t piece_rot_idx;                       // Current rotation index, 0..3
    uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH]; // Bottom-up board; 0xFF empty, otherwise a stored type
    uint8_t xoffset;                             // Left edge of this board in screen tiles
    uint8_t state;                               // GAME_STATE_* value
    uint32_t score;                              // Saturates at the maximum uint32_t value
    uint16_t lines_cleared;                      // Total cleared rows; saturates at 65535
    uint8_t clear_rows[20];                      // Flags for the twenty visible rows being wiped
    uint8_t clear_count;                         // Number of rows in the current clear
    uint8_t clear_row;                           // Current wipe row; 0xFF when no rows remain
    uint8_t clear_column;                        // Next cell to erase, 0..10
    uint8_t clear_tick;                          // Last wipe step, low KERNAL clock byte
    uint8_t pending_garbage;                     // Queued attack rows, applied before spawning
    uint8_t lose_row;                            // Next loss-fill row; 20 means finished
    uint8_t lose_tick;                           // Last loss-fill step, low KERNAL clock byte
    uint16_t piece_counts[7];                    // Spawn counts in I, O, T, S, Z, J, L order
    uint16_t clear_counts[4];                    // Counts of single, double, triple and four-row clears
    uint16_t level;                              // Zero-based level: lines_cleared / LINES_PER_LEVEL
    uint8_t gravity_tick;                        // Last downward step, low KERNAL clock byte
};

#endif // _GAMESTATE_H
