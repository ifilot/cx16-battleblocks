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

#ifndef _AI_H
#define _AI_H

#include <string.h>
#include <ascii_charmap.h>

#include "constants.h"
#include "pieces.h"
#include "gamestate.h"

/* Session setting: Easy (0), Normal (1), Hard (2). */
extern uint8_t ai_difficulty;

/*
 * Incremental computer-player search. Rank current placements, then search
 * the next piece for the four best candidates using board snapshots.
 */

struct PlayFieldEvaluation {
    uint32_t score[4][PLAYFIELD_WIDTH];          // Immediate placement scores; lower is better
    uint8_t pieces[4][4][2];                     // Normalized active-piece cells by rotation
    uint8_t rot_id;                              // Current immediate-search rotation
    uint8_t offset_id;                           // Current immediate-search column
    uint8_t best_rot_id;                         // Target rotation; 0xFF if no legal landing exists
    uint8_t best_offset_id;                      // Target leftmost board column
    uint8_t heights[PLAYFIELD_WIDTH];            // Column heights above the bottom row
    uint8_t row_counts[20];                      // Occupied cells in each visible row
    uint8_t widths[4];                           // Active-piece width for each rotation
    uint8_t unique_rotations;                    // Bit mask of distinct rotations to search
    uint8_t occupied;                            // Occupied visible cells in the snapshot
    uint8_t full_lines;                          // Complete visible rows in the snapshot
    uint8_t movement_stalls;                     // Consecutive blocked steering attempts
    uint32_t best_score;                         // Best combined current/preview score
    /* Four-candidate beam, expanded one preview placement per game-loop turn. */
    uint16_t rows[20];                           // Snapshot occupancy, one ten-bit mask per row
    uint16_t future_rows[20];                    // Simulated board after a shortlisted placement
    uint8_t future_heights[10];                  // Column heights of the simulated board
    uint8_t future_counts[20];                   // Row occupancy counts of the simulated board
    uint8_t future_occupied;                     // Occupied cells after simulated row removal
    uint8_t candidates[4];                       // Up to four placements, encoded rotation * 10 + column
    uint8_t candidate_count;                     // Number of shortlisted placements, 0..4
    uint8_t candidate_id;                        // Shortlisted placement being expanded
    uint8_t phase;                               // 0: immediate search, 1: preview search, 2: finished
    uint8_t next_type;                           // Preview tetromino type from the current bag
    uint8_t next_rot;                            // Preview rotation; 4 requests board preparation
    uint8_t next_offset;                         // Preview placement column
    uint32_t immediate_score;                    // Score of the current shortlisted placement
    uint32_t future_score;                       // Best preview score for that placement
};

extern struct PlayFieldEvaluation pfeval;        // Player-two search
extern struct PlayFieldEvaluation pfeval_player1; // Player-one demo search

/**
 * @brief Snapshot the board and reset the search for the active and next pieces
 *
 * @param gamestate  Player state to read or update
 * @param evaluation Search state belonging to this player
 */
void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

/**
 * @brief Advance one bounded search step; return whether the search is done
 *
 * @param gamestate  Player state to read or update
 * @param evaluation Search state belonging to this player
 *
 * @return One when both search stages are finished; zero while work remains
 */
uint8_t ai_calculate_next_score(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

/**
 * @brief Choose an AI_* action toward the best planned placement
 *
 * @param gamestate  Player state to read or update
 * @param evaluation Search state belonging to this player
 *
 * @return AI_ROTATE, AI_MOVEMENT_LEFT, AI_MOVEMENT_RIGHT or AI_DROP
 */
uint8_t ai_determine_movement(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);


#endif // _AI_H
