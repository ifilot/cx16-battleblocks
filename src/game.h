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

#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>
#include <string.h>

#include "ai.h"
#include "gamestate.h"
#include "pieces.h"
#include "bagofseven.h"
#include "screen.h"
#include "constants.h"

/*
 * Match rules and per-player updates: movement, gravity, row clears,
 * scoring and garbage attacks. Animated clears advance in the main loop.
 */

#define TYPE_TO_TILEID(type) (16 + (type))

extern uint8_t game_mode;            // Current GAME_MODE_* selection
extern struct GameState gs_player1;  // First player or first demo computer

/**
 * @brief Reset both player states and start a match in the requested mode
 *
 * @param mode One of the GAME_MODE_* values
 */
void game_init(uint8_t mode);

/**
 * @brief Apply queued garbage and spawn the next piece from the bag
 *
 * @param gamestate Player state to read or update
 */
void game_piece_new(struct GameState* gamestate);

/**
 * @brief Redraw the next-piece preview from the current bag
 *
 * @param gamestate Player state to read or update
 */
void game_update_next_pieces(struct GameState* gamestate);

/**
 * @brief Draw a four-by-three preview of a tetromino
 *
 * @param x    Horizontal tile coordinate
 * @param y    Vertical tile coordinate
 * @param type Tetromino type, 0..6 (I, O, T, S, Z, J, L)
 */
void game_place_piece(uint8_t x, uint8_t y, uint8_t type);

/**
 * @brief Move down one row, or lock the piece if it cannot descend
 *
 * @param gamestate Player state to read or update
 */
void game_piece_down(struct GameState* gamestate);

/**
 * @brief Move left or right if every destination cell is free
 *
 * @param gamestate Player state to read or update
 * @param direction MOVE_LEFT or MOVE_RIGHT
 */
void game_piece_move(struct GameState* gamestate, int8_t direction);

/**
 * @brief Store the active piece in the board occupancy array
 *
 * @param gamestate Player state to read or update
 */
void game_store_piece_playfield(struct GameState* gamestate);

/**
 * @brief Apply a previously validated movement and redraw the piece
 *
 * @param gamestate Player state to read or update
 * @param dx        Horizontal offset in board cells
 * @param dy        Vertical offset in board cells; negative moves downward
 */
void game_piece_apply_move(struct GameState* gamestate, int8_t dx, int8_t dy);

/**
 * @brief Rotate the piece if the destination is within bounds and free
 *
 * @param gamestate Player state to read or update
 */
void game_piece_rotate(struct GameState* gamestate);

/**
 * @brief Hard-drop to the shadow position and lock the piece
 *
 * @param gamestate Player state to read or update
 */
void game_piece_drop(struct GameState* gamestate);

/**
 * @brief Advance gravity for each active player when its interval expires
 */
void game_step();

/**
 * @brief Get the gravity interval in 60 Hz ticks, capped at level 13
 *
 * @param level Zero-based level, increasing every ten cleared lines
 *
 * @return Ticks per row, from 49 at level zero to 3 at level 13 and above
 */
uint8_t game_gravity_interval(uint16_t level);

/**
 * @brief Erase old positions and draw the landing shadow and active piece
 *
 * @param gamestate Player state to read or update
 */
void game_draw_block(struct GameState* gamestate);

/**
 * @brief Find complete visible rows and start their nonblocking wipe
 *
 * @param gamestate Player state to read or update
 */
void game_purge_rows(struct GameState* gamestate);

/**
 * @brief Advance row wipes and finish clears without delaying either player
 *
 * Advance each board's row wipe without delaying input or the other
 * player.
 */
void game_update_clears(void);

/**
 * @brief Recalculate the landing position for the current piece
 *
 * @param gamestate Player state to read or update
 */
void game_update_shadow(struct GameState* gamestate);

/**
 * @brief Apply a KERNAL keycode to the human players allowed by the mode
 *
 * @param keycode Keycode returned by cbm_k_getin()
 */
void game_parse_keyboard(uint8_t keycode);

/**
 * @brief Read the low byte of the KERNAL 60 Hz clock
 *
 * @return Low clock byte; wraps every 256 ticks
 */
uint8_t game_get_tick();

/**
 * @brief Mark the player as lost if blocks occupy the hidden spawn rows
 *
 * @param gamestate Player state to read or update
 */
void game_check_over(struct GameState* gamestate);

/**
 * @brief Initialize the nonblocking game-over fill animation
 *
 * @param gamestate Player state to read or update
 */
void game_loop_blockfill(struct GameState* gamestate);

/**
 * @brief Advance the game-over fill by one row when its tick expires
 *
 * @param gamestate Player state to read or update
 */
void game_update_blockfill(struct GameState* gamestate);

/**
 * @brief Check whether all players required by the mode are still active
 *
 * @return One while required players are running or clearing; zero after a loss
 */
uint8_t game_is_running();

/**
 * @brief Update the score, level and cleared-line HUD fields
 *
 * @param gamestate Player state to read or update
 */
void game_print_score(struct GameState* gamestate);

#endif // _GAME_H
