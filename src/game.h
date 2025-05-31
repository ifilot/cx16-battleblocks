#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>
#include <string.h>

#include "pieces.h"
#include "bagofseven.h"
#include "screen.h"
#include "constants.h"

#define TYPE_TO_TILEID(type) (16 + (type))

struct GameState {
    uint8_t player;
    uint8_t piece_old_position[4][2];
    uint8_t piece_position[4][2];
    uint8_t piece_old_shadow[4][2];
    uint8_t piece_shadow[4][2];
    struct PieceList piece_list;
    uint8_t piece_type;
    uint8_t piece_rot_idx;
    uint8_t playfield[22][10];
    uint8_t xoffset;
};

/**
 * @brief Initialize the game
 * 
 * Resets GameState structs for both players
 * 
 */
void game_init();

/**
 * @brief Take the next piece from the bag and introduce it to the game
 * 
 * @param gamestate player GameState
 */
void game_piece_new(struct GameState* gamestate);

/**
 * @brief Update the list of next pieces
 * 
 * @param gamestate player GameState
 */
void game_update_next_pieces(struct GameState* gamestate);

/**
 * @brief Place piece at position (x,y)
 * 
 * @param x x-position of the piece
 * @param y y-position of the piece
 * @param type piece type
 */
void game_place_piece(uint8_t x, uint8_t y, uint8_t type);

/**
 * @brief Try to move a piece downwards
 * 
 * @param gamestate player GameState
 */
void game_piece_down(struct GameState* gamestate);

/**
 * @brief Try to move a piece left or right
 * 
 * @param gamestate player GameState
 * @param direction MOVE_LEFT or MOVE_RIGHT
 */
void game_piece_move(struct GameState* gamestate, int8_t direction);

/**
 * @brief Function to store piece on the game field upon a collision
 * 
 * @param gamestate  player GameState
 */
void game_store_piece_playfield(struct GameState* gamestate);

/**
 * @brief Apply a movement of a piece, assume that validy is already checked
 * 
 * @param gamestate player GameState
 * @param dx x-increment
 * @param dy y-increment
 */
void game_piece_apply_move(struct GameState* gamestate, int8_t dx, int8_t dy);

/**
 * @brief Try to move a piece left or right
 * 
 * @param gamestate player GameState
 */
void game_piece_rotate(struct GameState* gamestate);

/**
 * @brief Drop the current piece
 * 
 * @param gamestate player GameState
 */
void game_piece_drop(struct GameState* gamestate);

/**
 * @brief Increment the game by one tick
 * 
 */
void game_step();

/**
 * @brief Draw the active blocks (both shadow and regular)
 * 
 * @param gamestate 
 */
void game_draw_block(struct GameState* gamestate);

/**
 * @brief Check whether any rows need purging and remove
 *        them from the game. Drop all rows above this row.
 * 
 */
void game_purge_rows(struct GameState* gamestate);

/**
 * @brief Show drop ouline
 * 
 */
void game_update_shadow(struct GameState* gamestate);

/**
 * @brief Parse keyboard input for the game
 * 
 * @param keycode keyboard input keycode
 */
void game_parse_keyboard(uint8_t keycode);

/**
 * @brief Get the current tick
 * 
 * @return uint8_t current tick
 */
uint8_t game_get_tick();

#endif