#ifndef _GAME_H
#define _GAME_H

#include <stdint.h>

#include "pieces.h"
#include "bagofseven.h"
#include "screen.h"
#include "constants.h"

struct GameState {
    uint8_t player;
    uint8_t piece_position[4][2];
    struct PieceList piece_list;
    uint8_t piece_type;
};

void game_init();

void game_place_upcoming_pieces(struct GameState* gamestate);

void game_place_piece(uint8_t x, uint8_t y, uint8_t type);

void game_new_piece(struct GameState* gamestate);

void game_piece_step(struct GameState* gamestate);

void game_step();

#endif