#include "game.h"

struct GameState gs_player1;
struct GameState gs_player2;

uint8_t piecepos_p1[4][2];
uint8_t piecepos_p2[4][2];

/**
 * @brief Initialize the game
 * 
 * Resets GameState structs for both players
 * 
 */
void game_init() {
    // initialize players
    gs_player1.player = PLAYER1;
    gs_player2.player = PLAYER2;

    // initialize upcoming tetris pieces
    bos_generate(&gs_player1.piece_list.bag1);
    bos_generate(&gs_player1.piece_list.bag2);
    bos_generate(&gs_player2.piece_list.bag1);
    bos_generate(&gs_player2.piece_list.bag2);

    memset(gs_player1.playfield, 0xFF, 22*10);
    memset(gs_player2.playfield, 0xFF, 22*10);

    memset(gs_player1.piece_shadow, 22, 8);
    memset(gs_player2.piece_shadow, 22, 8);

    gs_player1.xoffset = 2;
    gs_player2.xoffset = 22;

    gs_player1.state = GAME_STATE_RUN;
    gs_player2.state = GAME_STATE_RUN;

    game_piece_new(&gs_player1);
    game_piece_new(&gs_player2);
}

/**
 * @brief Take the next piece from the bag and introduce it to the game
 * 
 * @param gamestate player GameState
 */
void game_piece_new(struct GameState* gamestate) {
    uint8_t type = gamestate->piece_list.bag1.pieces[gamestate->piece_list.bag1.idx++];    
    uint8_t i;
    uint8_t offset = 0;

    gamestate->piece_type = type;
    gamestate->piece_rot_idx = 0;

    // set the new moving piece position
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] = 4 + pieces_start_table[type][i][0];
        gamestate->piece_position[i][1] = 20 - pieces_start_table[type][i][1];
    }

    // update the bag state
    if(gamestate->piece_list.bag1.idx == 7) {
        gamestate->piece_list.bag1 = gamestate->piece_list.bag2;
        bos_generate(&gamestate->piece_list.bag1);
        gamestate->piece_list.bag1.idx = 0;
    }

    // reset placeholders
    memset(gamestate->piece_shadow, 22, 8);
    memset(gamestate->piece_old_shadow, 22, 8);
    memset(gamestate->piece_old_position, 22, 8);

    // update the list of next pieces
    game_update_next_pieces(gamestate);
    game_update_shadow(gamestate);
    game_draw_block(gamestate);

    if(gamestate->player == PLAYER2) {
        ai_prepare(gamestate, &pfeval);
    }
}

/**
 * @brief Update the list of next pieces
 * 
 * @param gamestate player GameState
 */
void game_update_next_pieces(struct GameState* gamestate) {
    uint8_t pieces[4];
    uint8_t i;
    uint8_t idx;
    uint8_t xoffset = gamestate->player == PLAYER1 ? 14 : 34;
    struct PieceList *pl = &gamestate->piece_list;

    // clear all upcoming piece boxes
    screen_clear_rectangle(xoffset, 8, 4, 4);
    screen_clear_rectangle(xoffset, 14, 4, 12);

    // extract all new pieces from bags
    for(i=0; i<4; ++i) {
        idx = pl->bag1.idx + i;
        if(idx >= 7) {
            idx = pl->bag2.idx + i;
            pieces[i] = pl->bag2.pieces[idx];
        } else {
            pieces[i] = pl->bag1.pieces[idx];
        }
    }

    // position these new pieces in the boxes
    for(i=0; i<4; ++i) {
        if(i == 0) {
            game_place_piece(xoffset, 9, pieces[i]);
        } else {
            game_place_piece(xoffset, 11 + (i << 2), pieces[i]);
        }
    }
}

/**
 * @brief Place piece at position (x,y)
 * 
 * @param x x-position of the piece
 * @param y y-position of the piece
 * @param type piece type
 */
void game_place_piece(uint8_t x, uint8_t y, uint8_t type) {
    uint8_t i;
    for(i=0; i<4; i++) {
        screen_set_tile_layer1(x + pieces_start_table[type][i][0], y + pieces_start_table[type][i][1], TYPE_TO_TILEID(type));
    }
}

/**
 * @brief Try to move a piece downwards
 * 
 * @param gamestate  player GameState
 */
void game_piece_down(struct GameState* gamestate) {
    uint8_t i;
    uint8_t x,y;
    uint8_t xoffset = gamestate->player == PLAYER1 ? 2 : 22;
    uint8_t tile_id = TYPE_TO_TILEID(gamestate->piece_type);
    uint8_t place_piece = 0;

    // check whether a movement down results in piece placement
    for(i=0; i<4; ++i) {
        x = gamestate->piece_position[i][0];
        y = gamestate->piece_position[i][1];

        if(y == 0 || gamestate->playfield[y-1][x] != 0xFF) {
            place_piece = 1;
        }
    }

    if(place_piece == 1) {
        game_store_piece_playfield(gamestate);
        game_purge_rows(gamestate);
        game_piece_new(gamestate);
        game_check_over(gamestate);
        return;
    }

    game_piece_apply_move(gamestate, 0, -1);
}

/**
 * @brief Try to move a piece left or right
 * 
 * @param gamestate player GameState
 * @param direction MOVE_LEFT or MOVE_RIGHT
 */
void game_piece_move(struct GameState* gamestate, int8_t direction) {
    uint8_t i;
    int8_t x, y;
    uint8_t new_x;

    for(i=0; i<4; ++i) {
        x = gamestate->piece_position[i][0];
        y = gamestate->piece_position[i][1];

        new_x = x + direction;
        if ((uint8_t)new_x > 9 || gamestate->playfield[y][new_x] != 0xFF) {
            return;
        }
    }

    // apply movement
    game_piece_apply_move(gamestate, direction, 0);
}

/**
 * @brief Function to store piece on the game field upon a collision
 * 
 * @param gamestate  player GameState
 */
void game_store_piece_playfield(struct GameState* gamestate) {
    uint8_t i,x,y;

    for(i=0; i<4; ++i) {
        x = gamestate->piece_position[i][0];
        y = gamestate->piece_position[i][1];
        gamestate->playfield[y][x] = gamestate->piece_type;
    }
}

/**
 * @brief Apply a movement of a piece, assume that validy is already checked
 * 
 * @param gamestate player GameState
 * @param dx x-increment
 * @param dy y-increment
 */
void game_piece_apply_move(struct GameState* gamestate, int8_t dx, int8_t dy) {
    uint8_t i;
    uint8_t xoffset = (gamestate->player == PLAYER1) ? 2 : 22;
    uint8_t tile_id = TYPE_TO_TILEID(gamestate->piece_type);

    // Step 1: Remove piece
    memcpy(&gamestate->piece_old_position[0][0], &gamestate->piece_position[0][0], 8);

    // Step 2: Apply movement
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] += dx;
        gamestate->piece_position[i][1] += dy;
    }

    // perform shadow check and draw block
    game_update_shadow(gamestate);
    game_draw_block(gamestate);
}

/**
 * @brief Try to move a piece left or right
 * 
 * @param gamestate player GameState
 */
void game_piece_rotate(struct GameState* gamestate) {
    int8_t new_coords[4][2];
    uint8_t i;
    uint8_t type = gamestate->piece_type;
    uint8_t rot_id = gamestate->piece_rot_idx;
    uint8_t xoffset = (gamestate->player == PLAYER1) ? 2 : 22;
    uint8_t tile_id;

    // Step 1: Calculate new coordinates after rotation; if new position invalidates
    // playing field size; immediately exit function
    for(i=0; i<4; ++i) {
        new_coords[i][0] = gamestate->piece_position[i][0] + piece_rotation_table[type][rot_id][i][0];
        if(new_coords[i][0] < 0 || new_coords[i][0] > 9) {
            return;
        }
        new_coords[i][1] = gamestate->piece_position[i][1] - piece_rotation_table[type][rot_id][i][1];
        if(new_coords[i][1] < 0) {
            return;
        }
    }

    // Step 2: Check whether these new coordinates do not interfere; if interference,
    // immediately exit function
    for(i=0; i<4; ++i) {
        if(gamestate->playfield[new_coords[i][1]][new_coords[i][0]] != 0xFF) {
            return;
        }
    }

    // Step 3: Set old position (to be removed)
    memcpy(&gamestate->piece_old_position[0][0], &gamestate->piece_position[0][0], 8);

    // Step 4: Update gamepieces
    tile_id = TYPE_TO_TILEID(gamestate->piece_type);
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] = new_coords[i][0];
        gamestate->piece_position[i][1] = new_coords[i][1];
    }
    gamestate->piece_rot_idx = (gamestate->piece_rot_idx+1) & 0x03;

    // perform shadow check and draw block
    game_update_shadow(gamestate);
    game_draw_block(gamestate);
}

/**
 * @brief Drop the current piece
 * 
 * @param gamestate player GameState
 */
void game_piece_drop(struct GameState* gamestate) {
    memcpy(&gamestate->piece_old_position[0][0], &gamestate->piece_position[0][0], 8);
    memcpy(&gamestate->piece_position[0][0], &gamestate->piece_shadow[0][0], 8);
    game_draw_block(gamestate);
    game_store_piece_playfield(gamestate);
    game_purge_rows(gamestate);
    game_piece_new(gamestate);
}

/**
 * @brief Draw the active blocks (both shadow and regular)
 * 
 * @param gamestate 
 */
void game_draw_block(struct GameState* gamestate) {
    uint8_t i,x,y;

    // remove old blocks
    for(i=0; i<4; ++i) {
        y = gamestate->piece_old_shadow[i][1];
        if(y > 19) {
            continue;
        }
        x = gamestate->piece_old_shadow[i][0];
        screen_set_tile_layer1(gamestate->xoffset + x, 27 - y, TILE_NONE);

        y = gamestate->piece_old_position[i][1];
        if(y > 19) {
            continue;
        }
        x = gamestate->piece_old_position[i][0];
        screen_set_tile_layer1(gamestate->xoffset + x, 27 - y, TILE_NONE);
    }

    // first draw shadow block
    for(i=0; i<4; ++i) {
        y = gamestate->piece_shadow[i][1];
        if(y > 19) {
            continue;
        }
        x = gamestate->piece_shadow[i][0];
        screen_set_tile_layer1(gamestate->xoffset + x, 27 - y, TILE_SHADOW);
    }

    // then regular block
    for(i=0; i<4; ++i) {
        y = gamestate->piece_position[i][1];
        if(y > 19) {
            continue;
        }
        x = gamestate->piece_position[i][0];
        screen_set_tile_layer1(gamestate->xoffset + x, 27 - y, TYPE_TO_TILEID(gamestate->piece_type));
    }
}

/**
 * @brief Increment the game by one tick
 * 
 */
void game_step() {
    game_piece_down(&gs_player1);
    game_piece_down(&gs_player2);
}

/**
 * @brief Check whether any rows need purging and remove
 *        them from the game. Drop all rows above this row.
 * 
 */
void game_purge_rows(struct GameState* gamestate) {
    int8_t i;
    uint8_t j;
    uint8_t full;
    uint8_t update = 0;
    uint8_t xoffset = (gamestate->player == PLAYER1) ? 2 : 22;

    // update playing field
    for(i=19; i>=0; --i) {
        full = 1;
        for(j=0; j<10; ++j) {
            if(gamestate->playfield[i][j] == 0xFF) {
                full = 0;
                //break;
            }
        }

        if(full == 1 && i < 19) {
            memcpy(&gamestate->playfield[i][0], &gamestate->playfield[i+1][0], (19-i) * 10);
            memset(&gamestate->playfield[19][0], 0xFF, 10);
            update = 1;
        }
    }

    // redraw playing field
    if(update == 1) {
        screen_clear_rectangle(xoffset, 8, 10, 20);
        screen_redraw_playfield(xoffset, &gamestate->playfield[0][0]);
    }
}

/**
 * @brief Show drop ouline
 * 
 */
void game_update_shadow(struct GameState* gamestate) {
    uint8_t i,hit;
    int8_t row;
    uint8_t piece[4][2];
    uint8_t min = 20;
    uint8_t xoffset = (gamestate->player == PLAYER1) ? 2 : 22;

    // find minimum height of current piece
    for(i=0; i<4; i++) {
        if(gamestate->piece_position[i][1] < min) {
            min = gamestate->piece_position[i][1];
        }
    }

    // calculate block coordinates with lowest block at zero
    for(i=0; i<4; i++) {
        piece[i][0] = gamestate->piece_position[i][0];
        piece[i][1] = gamestate->piece_position[i][1] - min;
    }

    // find lowest position piece can be placed
    for(row=19; row>=0; row--) {
        hit = 0;
        for(i=0; i<4; i++) {
            if(gamestate->playfield[row+piece[i][1]][piece[i][0]] != 0xFF) {
                hit = 1;
                break;
            }
        }

        if(hit == 1 || row == 0) {
            if(hit == 1) {
                row++;
            }

            // copy current position to old shadow position
            memcpy(&gamestate->piece_old_shadow[0][0], &gamestate->piece_shadow[0][0], 8);

            // store new shadow
            for(i=0; i<4; i++) {
                gamestate->piece_shadow[i][0] = piece[i][0];
                gamestate->piece_shadow[i][1] = row + piece[i][1];
            }

            return;
        }
    }
}

/**
 * @brief Parse keyboard input for the game
 * 
 * @param keycode keyboard input keycode
 */
void game_parse_keyboard(uint8_t keycode) {
    switch(keycode) {
        case KEYCODE_DOWN:
            game_piece_down(&gs_player1);
        break;
        case KEYCODE_UP:
            game_piece_rotate(&gs_player1);
        break;
        case KEYCODE_LEFT:
            game_piece_move(&gs_player1, MOVE_LEFT);
        break;
        case KEYCODE_RIGHT:
            game_piece_move(&gs_player1, MOVE_RIGHT);
        break;
        case KEYCODE_0:
            game_piece_drop(&gs_player1);
        break;
    }
}

/**
 * @brief Get the current tick
 * 
 * @return uint8_t current tick
 */
uint8_t game_get_tick() {
    static uint8_t t;
    asm("jsr $FFDE");
    asm("sta %v", t);
    return t;
}

/**
 * @brief Check whether game is over
 * 
 */
void game_check_over(struct GameState* gamestate) {
    uint8_t i;
    uint8_t* ptr = &gamestate->playfield[20][0];

    for(i=0; i<20; i++) {
        if(*(ptr++) != 0xFF) {
            gamestate->state = GAME_STATE_LOST;
            game_loop_blockfill(gamestate);
        }
    }
}

/**
 * @brief Play a small animation where the playing field gets filled with blocks
 * 
 * @param gamestate 
 */
void game_loop_blockfill(struct GameState* gamestate) {
    uint8_t start, now;
    uint8_t i,j;

    for(j=0; j<20; j++) {
        start = game_get_tick();
        now = start;
        while((now - start) < 1) {
            now = game_get_tick();
        }
        for(i=0; i<10; ++i) {
            screen_set_tile_layer1(gamestate->xoffset + i, 27 - j, TILE_ENDGAME);
        }
    }
}

/**
 * @brief Check whether game is still running
 * 
 * @return uint8_t 1 if still running, 0 otherwise
 */
uint8_t game_is_running() {
    if(gs_player1.state == GAME_STATE_RUN) {
        return 1;
    } else {
        return 0;
    }
}