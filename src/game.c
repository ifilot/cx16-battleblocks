#include "game.h"

struct GameState gs_player1;
struct GameState gs_player2;

uint8_t piecepos_p1[4][2];
uint8_t piecepos_p2[4][2];

void game_init() {
    // initialize players
    gs_player1.player = PLAYER1;
    gs_player2.player = PLAYER2;

    // initialize upcoming tetris pieces
    bos_generate(&gs_player1.piece_list.bag1);
    bos_generate(&gs_player1.piece_list.bag2);
    bos_generate(&gs_player2.piece_list.bag1);
    bos_generate(&gs_player2.piece_list.bag2);

    game_place_upcoming_pieces(&gs_player1);
    game_place_upcoming_pieces(&gs_player2);

    game_new_piece(&gs_player1);
    game_new_piece(&gs_player2);
}

void game_place_upcoming_pieces(struct GameState* gamestate) {
    uint8_t pieces[4];
    uint8_t i;
    uint8_t idx;
    uint8_t xoffset = gamestate->player == PLAYER1 ? 14 : 34;
    struct PieceList *pl = &gamestate->piece_list;

    for(i=0; i<4; ++i) {
        idx = pl->bag1.idx + i;
        if(idx >= 7) {
            idx = pl->bag2.idx + i;
            pieces[i] = pl->bag2.pieces[idx];
        } else {
            pieces[i] = pl->bag1.pieces[idx];
        }
    }

    for(i=0; i<4; ++i) {
        if(i == 0) {
            game_place_piece(xoffset, 9, pieces[i]);
        } else {
            game_place_piece(xoffset, 11 + (i << 2), pieces[i]);
        }
    }
}

void game_place_piece(uint8_t x, uint8_t y, uint8_t type) {
    uint8_t i;
    for(i=0; i<4; i++) {
        screen_set_tile_layer1(x + pieces_start[type][i][0], y + pieces_start[type][i][1], 13 + type);
    }
}

void game_new_piece(struct GameState* gamestate) {
    uint8_t type = gamestate->piece_list.bag1.pieces[gamestate->piece_list.bag1.idx++];    
    uint8_t i;
    uint8_t offset = 0;

    gamestate->piece_type = type;

    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] = 4 + pieces_start[type][i][0];
        gamestate->piece_position[i][1] = 21 - pieces_start[type][i][1];
    }

    if(gamestate->piece_list.bag1.idx == 7) {
        gamestate->piece_list.bag1 = gamestate->piece_list.bag2;
        bos_generate(&gamestate->piece_list.bag1);
        gamestate->piece_list.bag1.idx = 0;
    }

    offset = gamestate->player == PLAYER1 ? 14 : 34;
    screen_clear_rectangle(offset, 8, 4, 4);
    screen_clear_rectangle(offset, 14, 4, 12);
    game_place_upcoming_pieces(gamestate);
}

void game_piece_step(struct GameState* gamestate) {
    uint8_t i;
    uint8_t x,y;
    uint8_t xoffset = gamestate->player == PLAYER1 ? 2 : 22;
    uint8_t tile_id = 13 + gamestate->piece_type;
    uint8_t newpiece = 0;

    for(i=0; i<4; ++i) {
        x = gamestate->piece_position[i][0];
        y = gamestate->piece_position[i][1]--;

        if(y < 20) {
            screen_set_tile_layer1(xoffset + x, 27 - y, TILE_NONE);
        }
    }

    for(i=0; i<4; ++i) {
        x = gamestate->piece_position[i][0];
        y = gamestate->piece_position[i][1];

        if(y == 0) {
            newpiece = 1;
        }

        if(y < 20) {
            screen_set_tile_layer1(xoffset + x, 27 - y, tile_id);
        }
    }

    if(newpiece == 1) {
        game_new_piece(gamestate);
    }
}

void game_step() {
    game_piece_step(&gs_player1);
    game_piece_step(&gs_player2);
}