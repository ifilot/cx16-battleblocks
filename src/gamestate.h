#ifndef _GAMESTATE_H
#define _GAMESTATE_H

#include <stdint.h>

#include "pieces.h"
#include "bagofseven.h"

extern struct GameState gs_player2;

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
    uint8_t state;
};

#endif // _GAMESTATE_H