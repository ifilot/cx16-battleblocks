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

#include "game.h"
#include "sound.h"
#include <stdio.h>

/*
 * Match rules and per-player updates: movement, gravity, row clears,
 * scoring and garbage attacks. Animated clears advance in the main loop.
 */

struct GameState gs_player1;
struct GameState gs_player2;
uint8_t game_mode;

/**
 * @brief Reset both player states and start a match in the requested mode
 *
 * @param mode One of the GAME_MODE_* values
 */
void game_init(uint8_t mode) {
    game_mode = mode;
    // initialize players
    gs_player1.player = PLAYER1;
    gs_player2.player = PLAYER2;

    // Prepare the current and next bags for both players.
    bos_generate(&gs_player1.piece_list.bag1);
    bos_generate(&gs_player1.piece_list.bag2);
    bos_generate(&gs_player2.piece_list.bag1);
    bos_generate(&gs_player2.piece_list.bag2);

    memset(gs_player1.playfield, 0xFF, PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT);
    memset(gs_player2.playfield, 0xFF, PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT);

    memset(gs_player1.piece_shadow, 22, 8);
    memset(gs_player2.piece_shadow, 22, 8);

    gs_player1.xoffset = 1;
    gs_player2.xoffset = 21;

    gs_player1.pending_garbage = 0;
    gs_player2.pending_garbage = 0;
    gs_player1.lose_row = 20;
    gs_player2.lose_row = 20;
    gs_player1.clear_count = 0;
    gs_player2.clear_count = 0;
    gs_player1.score = 0;
    gs_player2.score = 0;
    gs_player1.lines_cleared = 0;
    gs_player2.lines_cleared = 0;
    gs_player1.level = 0;
    gs_player2.level = 0;
    memset(gs_player1.piece_counts, 0, sizeof(gs_player1.piece_counts));
    memset(gs_player2.piece_counts, 0, sizeof(gs_player2.piece_counts));
    memset(gs_player1.clear_counts, 0, sizeof(gs_player1.clear_counts));
    memset(gs_player2.clear_counts, 0, sizeof(gs_player2.clear_counts));
    {
        uint8_t i;
        for(i = 0; i < 7; i++) {
            screen_draw_hud_number(PLAYER1, HUD_PIECE_BASE + i, 0);
            if(mode != GAME_MODE_SINGLE) screen_draw_hud_number(PLAYER2, HUD_PIECE_BASE + i, 0);
        }
        for(i = 0; i < 4; i++) {
            screen_draw_hud_number(PLAYER1, HUD_CLEAR_BASE + i, 0);
            if(mode != GAME_MODE_SINGLE) screen_draw_hud_number(PLAYER2, HUD_CLEAR_BASE + i, 0);
        }
    }

    gs_player1.state = GAME_STATE_RUN;
    gs_player2.state = mode == GAME_MODE_SINGLE ? GAME_STATE_PAUSE : GAME_STATE_RUN;

    game_piece_new(&gs_player1);
    if(mode != GAME_MODE_SINGLE) game_piece_new(&gs_player2);

    game_print_score(&gs_player1);
    if(mode != GAME_MODE_SINGLE) game_print_score(&gs_player2);
}

/**
 * @brief Queue the completed clear's attack on the opposing board
 *
 * Attacks wait for a spawn boundary; they never move a falling piece or
 * interfere with a board's row wipe. Cap the queue at a whole field.
 */
static void game_send_garbage(struct GameState* sender) {
    const uint8_t attacks[5] = {0, 0, 1, 2, 4};
    struct GameState* receiver;
    uint8_t total;
    if(game_mode == GAME_MODE_SINGLE || sender->clear_count < 2) return;
    receiver = sender->player == PLAYER1 ? &gs_player2 : &gs_player1;
    total = receiver->pending_garbage + attacks[sender->clear_count > 4 ? 4 : sender->clear_count];
    receiver->pending_garbage = total > PLAYFIELD_HEIGHT ? PLAYFIELD_HEIGHT : total;
    PLAYER_SOUND(sender, SFX_GARBAGE_SEND);
}

/**
 * @brief Apply queued rows at a spawn boundary and detect overflow
 */
static void game_receive_garbage(struct GameState* gamestate) {
    uint8_t rows = gamestate->pending_garbage;
    uint8_t row, col, hole, overflow = 0;
    if(!rows) return;
    gamestate->pending_garbage = 0;
    PLAYER_SOUND(gamestate, SFX_GARBAGE_RECV);
    // Remember blocks pushed out of memory, so they cannot silently vanish.
    for(row = PLAYFIELD_HEIGHT - rows; row < PLAYFIELD_HEIGHT; row++) {
        for(col = 0; col < PLAYFIELD_WIDTH; col++) {
            if(gamestate->playfield[row][col] != 0xFF) overflow = 1;
        }
    }
    memmove((uint8_t*)gamestate->playfield + rows * PLAYFIELD_WIDTH, gamestate->playfield[0],
            (PLAYFIELD_HEIGHT - rows) * PLAYFIELD_WIDTH);
    for(row = 0; row < rows; row++) {
        // Each row rolls its own gap; neighbouring rows may match by chance.
        hole = rand() % PLAYFIELD_WIDTH;
        for(col = 0; col < PLAYFIELD_WIDTH; col++) {
            gamestate->playfield[row][col] = col == hole ? 0xFF :
                ((rand() & 1) ? TYPE_GARBAGE_DARK : TYPE_GARBAGE_LIGHT);
        }
    }
    screen_redraw_playfield(gamestate->xoffset, &gamestate->playfield[0][0]);
    if(overflow) {
        gamestate->state = GAME_STATE_LOST;
        game_loop_blockfill(gamestate);
    } else {
        game_check_over(gamestate);
    }
}

/**
 * @brief Apply queued garbage and spawn the next piece from the bag
 *
 * @param gamestate Player state to read or update
 */
void game_piece_new(struct GameState* gamestate) {
    uint8_t type;
    uint8_t i;

    game_receive_garbage(gamestate);
    if(gamestate->state == GAME_STATE_LOST) return;
    type = gamestate->piece_list.bag1.pieces[gamestate->piece_list.bag1.idx++];
    gamestate->piece_type = type;
    if(gamestate->piece_counts[type] != 65535U) gamestate->piece_counts[type]++;
    screen_draw_hud_number(gamestate->player, HUD_PIECE_BASE + type, gamestate->piece_counts[type]);
    gamestate->piece_rot_idx = 0;

    // set the new moving piece position
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] = 4 + pieces_start_table[type][i][0];
        gamestate->piece_position[i][1] = 20 - pieces_start_table[type][i][1];
    }

    // update the bag state
    if(gamestate->piece_list.bag1.idx == 7) {
        gamestate->piece_list.bag1 = gamestate->piece_list.bag2;
        bos_generate(&gamestate->piece_list.bag2);
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

    if(game_mode == GAME_MODE_DEMO ||
       (gamestate->player == PLAYER2 && game_mode == GAME_MODE_COMPUTER)) {
        ai_prepare(gamestate, gamestate->player == PLAYER1 ? &pfeval_player1 : &pfeval);
    }
    gamestate->gravity_tick = game_get_tick();
}

/**
 * @brief Redraw the next-piece preview from the current bag
 *
 * @param gamestate Player state to read or update
 */
void game_update_next_pieces(struct GameState* gamestate) {
    uint8_t xoffset = gamestate->player == PLAYER1 ? 14 : 34;
    struct PieceList *pl = &gamestate->piece_list;

    // clear all upcoming piece boxes
    screen_clear_rectangle(xoffset, 10, 4, 3);

    // draw piece
    game_place_piece(xoffset, 10, pl->bag1.pieces[pl->bag1.idx]);
}

/**
 * @brief Draw a four-by-three preview of a tetromino
 *
 * @param x    Horizontal tile coordinate
 * @param y    Vertical tile coordinate
 * @param type Tetromino type, 0..6 (I, O, T, S, Z, J, L)
 */
void game_place_piece(uint8_t x, uint8_t y, uint8_t type) {
    uint8_t i,j;
    uint32_t map_base_addr;
    const uint8_t* ptr = &piece_next_drawing_table[type][0];

    for(i=0; i<3; i++) {
        map_base_addr = MAPBASE1 + ((y+i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= (1 << 5);

        for(j=0; j<4; j++) {
            VERA.data0 = *(ptr++);
        }
    }
}

/**
 * @brief Move down one row, or lock the piece if it cannot descend
 *
 * @param gamestate Player state to read or update
 */
void game_piece_down(struct GameState* gamestate) {
    uint8_t i;
    uint8_t x,y;
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
        PLAYER_SOUND(gamestate, SFX_LOCK);
        game_store_piece_playfield(gamestate);
        game_purge_rows(gamestate);
        if(gamestate->state == GAME_STATE_RUN) {
            game_piece_new(gamestate);
            game_check_over(gamestate);
        }
        return;
    }

    game_piece_apply_move(gamestate, 0, -1);
    gamestate->gravity_tick = game_get_tick();
}

/**
 * @brief Move left or right if every destination cell is free
 *
 * @param gamestate Player state to read or update
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
    MOTION_SOUND(gamestate, SFX_MOVE);
}

/**
 * @brief Store the active piece in the board occupancy array
 *
 * @param gamestate Player state to read or update
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
 * @brief Apply a previously validated movement and redraw the piece
 *
 * @param gamestate Player state to read or update
 * @param dx        Horizontal offset in board cells
 * @param dy        Vertical offset in board cells; negative moves downward
 */
void game_piece_apply_move(struct GameState* gamestate, int8_t dx, int8_t dy) {
    uint8_t i;

    // Preserve the previous position so the redraw can erase it.
    memcpy(&gamestate->piece_old_position[0][0], &gamestate->piece_position[0][0], 8);

    // Step 2: Apply movement
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] += dx;
        gamestate->piece_position[i][1] += dy;
    }

    // Vertical movement changes neither the board nor the landing position.
    if(dx) game_update_shadow(gamestate);
    else memcpy(gamestate->piece_old_shadow, gamestate->piece_shadow, 8);
    game_draw_block(gamestate);
}

/**
 * @brief Rotate the piece if the destination is within bounds and free
 *
 * @param gamestate Player state to read or update
 */
void game_piece_rotate(struct GameState* gamestate) {
    int8_t new_coords[4][2];
    uint8_t i;
    uint8_t type = gamestate->piece_type;
    uint8_t rot_id = gamestate->piece_rot_idx;

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
    for(i=0; i<4; ++i) {
        gamestate->piece_position[i][0] = new_coords[i][0];
        gamestate->piece_position[i][1] = new_coords[i][1];
    }
    gamestate->piece_rot_idx = (gamestate->piece_rot_idx+1) & 0x03;

    // perform shadow check and draw block
    game_update_shadow(gamestate);
    game_draw_block(gamestate);
    MOTION_SOUND(gamestate, SFX_ROTATE);
}

/**
 * @brief Hard-drop to the shadow position and lock the piece
 *
 * @param gamestate Player state to read or update
 */
void game_piece_drop(struct GameState* gamestate) {
    PLAYER_SOUND(gamestate, SFX_HARDDROP);
    memcpy(&gamestate->piece_old_position[0][0], &gamestate->piece_position[0][0], 8);
    memcpy(&gamestate->piece_position[0][0], &gamestate->piece_shadow[0][0], 8);
    game_draw_block(gamestate);
    game_store_piece_playfield(gamestate);
    game_purge_rows(gamestate);
    if(gamestate->state == GAME_STATE_RUN) {
        game_piece_new(gamestate);
        game_check_over(gamestate);
    }
}

/**
 * @brief Erase old positions and draw the landing shadow and active piece
 *
 * @param gamestate Player state to read or update
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
 * @brief Get the gravity interval in 60 Hz ticks, capped at level 13
 *
 * @param level Zero-based level, increasing every ten cleared lines
 *
 * @return Ticks per row, from 49 at level zero to 3 at level 13 and above
 */
uint8_t game_gravity_interval(uint16_t level) {
    /* 60 Hz ticks per row. Preserve the old opening pace, then accelerate.
     * Level 13 and above cap at 20 rows/sec, leaving time for input and AI.
     */
    static const uint8_t intervals[] = {49, 43, 38, 33, 28, 23, 18, 13, 10, 8, 6, 5, 4, 3};
    return level >= sizeof(intervals) ? 3 : intervals[level];
}

/**
 * @brief Advance one player's gravity without blocking input or animations
 */
static void game_update_gravity(struct GameState* player) {
    uint8_t now = game_get_tick();
    if(player->state != GAME_STATE_RUN) {
        player->gravity_tick = now;
        return;
    }
    if((uint8_t)(now - player->gravity_tick) >= game_gravity_interval(player->level)) {
        /* One step at a time: input and row wipes remain in the main loop. */
        game_piece_down(player);
    }
}

/**
 * @brief Advance gravity for each active player when its interval expires
 */
void game_step() {
    game_update_gravity(&gs_player1);
    if(game_mode != GAME_MODE_SINGLE && game_is_running()) game_update_gravity(&gs_player2);
}

/**
 * @brief Find complete visible rows and start their nonblocking wipe
 *
 * @param gamestate Player state to read or update
 */
void game_purge_rows(struct GameState* gamestate) {
    uint8_t row, col;
    gamestate->clear_count = 0;
    for(row = 0; row < 20; row++) {
        gamestate->clear_rows[row] = 1;
        for(col = 0; col < PLAYFIELD_WIDTH; col++) {
            if(gamestate->playfield[row][col] == 0xFF) {
                gamestate->clear_rows[row] = 0;
                break;
            }
        }
        gamestate->clear_count += gamestate->clear_rows[row];
    }
    if(!gamestate->clear_count) return;
    sound_clear(gamestate->player, gamestate->clear_count);
    gamestate->state = GAME_STATE_CLEAR;
    gamestate->clear_row = 19;
    while(!gamestate->clear_rows[gamestate->clear_row]) gamestate->clear_row--;
    gamestate->clear_column = 0;
    gamestate->clear_tick = game_get_tick();
}

/**
 * @brief Compact wiped rows, award score and garbage, then spawn a piece
 */
static void game_finish_clear(struct GameState* gamestate) {
    uint8_t row, destination = 0;
    uint16_t old_level = gamestate->level;
    uint32_t award;
    const uint16_t score_incr4[5] = {0, 40, 100, 300, 1200};
    // Compact all cleared rows together after their individual wipes finish.
    for(row = 0; row < PLAYFIELD_HEIGHT; row++) {
        if(row < 20 && gamestate->clear_rows[row]) continue;
        if(destination != row) {
            memcpy(gamestate->playfield[destination], gamestate->playfield[row], PLAYFIELD_WIDTH);
        }
        destination++;
    }
    while(destination < PLAYFIELD_HEIGHT) {
        memset(gamestate->playfield[destination++], 0xFF, PLAYFIELD_WIDTH);
    }
    screen_redraw_playfield(gamestate->xoffset, &gamestate->playfield[0][0]);
    /* The finishing clear earns the old level's multiplier. Saturate totals
     * rather than wrapping a long match back to level zero or score zero.
     */
    award = (uint32_t)score_incr4[gamestate->clear_count > 4 ? 4 : gamestate->clear_count]
        * ((uint32_t)gamestate->level + 1);
    if(0xFFFFFFFFUL - gamestate->score < award) gamestate->score = 0xFFFFFFFFUL;
    else gamestate->score += award;
    if((uint16_t)(65535U - gamestate->lines_cleared) < gamestate->clear_count) {
        gamestate->lines_cleared = 65535U;
    } else gamestate->lines_cleared += gamestate->clear_count;
    gamestate->level = gamestate->lines_cleared / LINES_PER_LEVEL;
    if(gamestate->clear_count <= 4) {
        uint8_t clear = gamestate->clear_count - 1;
        if(gamestate->clear_counts[clear] != 65535U) gamestate->clear_counts[clear]++;
        screen_draw_hud_number(gamestate->player, HUD_CLEAR_BASE + clear, gamestate->clear_counts[clear]);
    }
    game_send_garbage(gamestate);
    if(gamestate->level > old_level) PLAYER_SOUND(gamestate, SFX_LEVELUP);
    gamestate->clear_count = 0;
    gamestate->state = GAME_STATE_RUN;
    game_print_score(gamestate);
    game_piece_new(gamestate);
    game_check_over(gamestate);
}

/**
 * @brief Erase the next cell in a player's completed-row wipe
 */
static void game_update_clear(struct GameState* gamestate) {
    uint8_t now;
    if(gamestate->state != GAME_STATE_CLEAR) return;
    now = game_get_tick();
    if((uint8_t)(now - gamestate->clear_tick) < 2) return;
    gamestate->clear_tick = now;
    if(gamestate->clear_column == PLAYFIELD_WIDTH) {
        // Work downwards through the completed rows, one row at a time.
        do {
            gamestate->clear_row--;
        } while(gamestate->clear_row != 0xFF && !gamestate->clear_rows[gamestate->clear_row]);
        if(gamestate->clear_row == 0xFF) {
            game_finish_clear(gamestate);
            return;
        }
        gamestate->clear_column = 0;
    }
    screen_clear_row_cell(gamestate->xoffset, gamestate->clear_row, gamestate->clear_column++);
}

/**
 * @brief Advance row wipes and finish clears without delaying either player
 */
void game_update_clears(void) {
    game_update_clear(&gs_player1);
    if(game_mode != GAME_MODE_SINGLE && gs_player1.state != GAME_STATE_LOST) {
        game_update_clear(&gs_player2);
    }
}

/**
 * @brief Recalculate the landing position for the current piece
 *
 * @param gamestate Player state to read or update
 */
void game_update_shadow(struct GameState* gamestate) {
    uint8_t i, height, x, relative_y, landing = 0, min = 23;
    int8_t required;
    uint8_t* cell;
    for(i = 0; i < 4; i++) {
        if(gamestate->piece_position[i][1] < min) min = gamestate->piece_position[i][1];
    }
    for(i = 0; i < 4; i++) {
        x = gamestate->piece_position[i][0];
        relative_y = gamestate->piece_position[i][1] - min;
        cell = &gamestate->playfield[19][x];
        height = 20;
        while(height && *cell == 0xFF) {
            height--;
            if(height) cell -= PLAYFIELD_WIDTH;
        }
        required = height - relative_y;
        if(required > (int8_t)landing) landing = required;
    }
    memcpy(gamestate->piece_old_shadow, gamestate->piece_shadow, 8);
    for(i = 0; i < 4; i++) {
        gamestate->piece_shadow[i][0] = gamestate->piece_position[i][0];
        gamestate->piece_shadow[i][1] = landing + gamestate->piece_position[i][1] - min;
    }
}

/**
 * @brief Apply a KERNAL keycode to the human players allowed by the mode
 *
 * @param keycode Keycode returned by cbm_k_getin()
 */
void game_parse_keyboard(uint8_t keycode) {
    if(game_mode == GAME_MODE_DEMO) return;
    if(gs_player1.state == GAME_STATE_RUN) switch(keycode) {
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
        case KEYCODE_SPACE:
            game_piece_drop(&gs_player1);
        break;
    }

    if(game_mode != GAME_MODE_LOCAL || gs_player2.state != GAME_STATE_RUN) return;
    switch(keycode) {
        case 'A': case 'a':
            game_piece_move(&gs_player2, MOVE_LEFT);
            break;
        case 'D': case 'd':
            game_piece_move(&gs_player2, MOVE_RIGHT);
            break;
        case 'S': case 's':
            game_piece_down(&gs_player2);
            break;
        case 'W': case 'w':
            game_piece_rotate(&gs_player2);
            break;
        case 'Q': case 'q':
            game_piece_drop(&gs_player2);
            break;
    }
}

/**
 * @brief Read the low byte of the KERNAL 60 Hz clock
 *
 * @return Low clock byte; wraps every 256 ticks
 */
uint8_t game_get_tick() {
    static uint8_t t;
    asm("jsr $FFDE");
    asm("sta %v", t);
    return t;
}

/**
 * @brief Mark the player as lost if blocks occupy the hidden spawn rows
 *
 * @param gamestate Player state to read or update
 */
void game_check_over(struct GameState* gamestate) {
    uint8_t i;
    uint8_t* ptr = &gamestate->playfield[20][0];

    if(gamestate->state == GAME_STATE_LOST) return;
    for(i=0; i<40; i++) {
        if(*(ptr++) != 0xFF) {
            gamestate->state = GAME_STATE_LOST;
            game_loop_blockfill(gamestate);
            return;
        }
    }
}

/**
 * @brief Initialize the nonblocking game-over fill animation
 *
 * @param gamestate Player state to read or update
 */
void game_loop_blockfill(struct GameState* gamestate) {
    gamestate->lose_row = 0;
    gamestate->lose_tick = game_get_tick();
}

/**
 * @brief Advance the game-over fill by one row when its tick expires
 *
 * @param gamestate Player state to read or update
 */
void game_update_blockfill(struct GameState* gamestate) {
    uint8_t i, now;
    if(gamestate->state != GAME_STATE_LOST || gamestate->lose_row >= 20) return;
    now = game_get_tick();
    if((uint8_t)(now - gamestate->lose_tick) < 1) return;
    gamestate->lose_tick = now;
    for(i = 0; i < 10; i++) {
        screen_set_tile_layer1(gamestate->xoffset + i, 27 - gamestate->lose_row, TILE_ENDGAME);
    }
    gamestate->lose_row++;
}

/**
 * @brief Check whether all players required by the mode are still active
 *
 * @return One while required players are running or clearing; zero after a loss
 */
uint8_t game_is_running() {
    return (gs_player1.state == GAME_STATE_RUN || gs_player1.state == GAME_STATE_CLEAR) &&
        (game_mode == GAME_MODE_SINGLE || gs_player2.state == GAME_STATE_RUN ||
         gs_player2.state == GAME_STATE_CLEAR);
}

/**
 * @brief Update the score, level and cleared-line HUD fields
 *
 * @param gamestate Player state to read or update
 */
void game_print_score(struct GameState* gamestate) {
    screen_draw_hud_number(gamestate->player, HUD_SCORE, gamestate->score);
    screen_draw_hud_number(gamestate->player, HUD_LEVEL, gamestate->level);
    screen_draw_hud_number(gamestate->player, HUD_LINES, gamestate->lines_cleared);
}
