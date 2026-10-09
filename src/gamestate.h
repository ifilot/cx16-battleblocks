#ifndef _GAMESTATE_H
#define _GAMESTATE_H

#include <stdint.h>

#include "pieces.h"
#include "bagofseven.h"
#include "constants.h"

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
    uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH];
    uint8_t xoffset;
    uint8_t state;
    uint32_t score;
    uint16_t lines_cleared;
    uint8_t clear_rows[20];
    uint8_t clear_count;
    uint8_t clear_row;
    uint8_t clear_column;
    uint8_t clear_tick;
    uint8_t pending_garbage;
    uint8_t lose_row;
    uint8_t lose_tick;
    uint16_t piece_counts[7];
    uint16_t clear_counts[4];
    uint16_t level;
    uint8_t gravity_tick;
};

#endif // _GAMESTATE_H
