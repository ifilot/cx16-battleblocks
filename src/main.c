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

#include <stdio.h>
#include "screen.h"
#include "game.h"
#include "ai.h"
#include "menu.h"
#include "sound.h"

/*
 * Application entry point and scene loop. Coordinates the title, menus,
 * matches and results while keeping keyboard input and AI work responsive.
 */

static uint8_t game_tiles_loaded;

/**
 * @brief Load the splash and wait for Enter or Space
 */
static void show_title(void) {
    uint8_t key;
    game_tiles_loaded = 0;
    screen_init_title();
    screen_load_tiles("titlepal.dat", 0x1FA00UL);
    screen_load_tiles("title.dat", TILEBASE);
    screen_load_tiles("titlemap.dat", TITLE_MAPBASE);
    screen_apply_title_style();
    VERA.display.video = 0x11;
    sound_title();
    do {
        key = cbm_k_getin();
    } while(key != KEYCODE_SPACE && key != KEYCODE_RETURN);
    sound_menu_effect(SFX_START);
}

/**
 * @brief Load shared tile assets when needed and prepare the menu layers
 */
static void show_menu(void) {
    screen_transition_begin();
    if(!game_tiles_loaded) {
        screen_load_tiles("tiles.dat", TILEBASE);
        screen_load_tiles("cyber.dat", (uint32_t)TILE_CYBERPUNK_FIRST * 256);
        game_tiles_loaded = 1;
    }
    screen_fill_layer(LAYER0, TILE_BG0);
    screen_fill_layer(LAYER1, TILE_NONE);
    screen_init_menu();
    sound_title();
}

/**
 * @brief Prepare the selected game mode, HUD and player banners
 */
static void show_game(uint8_t mode) {
    uint8_t players = mode == GAME_MODE_SINGLE ? 1 : 2;
    screen_transition_begin();
    screen_fill_layer(LAYER0, TILE_BG0);
    screen_fill_layer(LAYER1, TILE_NONE);
    screen_init_game();
    VERA.display.video = 0x41;
    screen_create_background(players);
    screen_create_scorebox(players);
    screen_draw_next_boxes(players);
    game_init(mode);
    screen_draw_player_banner(PLAYER1, mode == GAME_MODE_DEMO, 1);
    menu_settext(2, 29, mode == GAME_MODE_DEMO ? "DEMO MODE" : "ARROWS SPACE DROP");
    if(mode == GAME_MODE_SINGLE) {
        menu_settext(22, 2, "SINGLE PLAYER");
        menu_settext(22, 8, "CONTROLS");
        menu_settext(22, 11, "ARROWS MOVE");
        menu_settext(22, 13, "UP ROTATES");
        menu_settext(22, 15, "SPACE DROPS");
        menu_settext(22, 17, "0 ALSO DROPS");
        menu_settext(22, 21, "ESC FOR MENU");
    } else if(mode == GAME_MODE_DEMO) {
        screen_draw_player_banner(PLAYER2, 1, 2);
        menu_settext(22, 29, "ESC FOR MENU");
    } else if(mode == GAME_MODE_COMPUTER) {
        screen_draw_player_banner(PLAYER2, 1, 1);
        menu_settext(22, 29, "ESC FOR MENU");
    } else {
        screen_draw_player_banner(PLAYER2, 0, 2);
        menu_settext(22, 29, "WASD Q DROP");
    }
    screen_transition_end(0x31);
    gs_player1.gravity_tick = gs_player2.gravity_tick = game_get_tick();
    sound_battle();
}

/**
 * @brief Advance one AI search step and apply a rate-limited movement
 */
static void step_computer(struct GameState* player,
                          struct PlayFieldEvaluation* evaluation, uint8_t* last_tick) {
    uint8_t movement, old_x, old_rotation, search_done;
    if(player->state != GAME_STATE_RUN) return;
    search_done = ai_calculate_next_score(player, evaluation);
    if((!search_done && evaluation->best_rot_id == 0xFF) ||
       (uint8_t)(game_get_tick() - *last_tick) < 2) return;
    old_x = player->piece_position[0][0];
    old_rotation = player->piece_rot_idx;
    movement = ai_determine_movement(player, evaluation);
    // Steer during lookahead, but commit a hard drop after the full search.
    if(!search_done && movement == AI_DROP) return;
    switch(movement) {
        case AI_MOVEMENT_LEFT: game_piece_move(player, -1); break;
        case AI_MOVEMENT_RIGHT: game_piece_move(player, 1); break;
        case AI_ROTATE: game_piece_rotate(player); break;
        case AI_MOVEMENT_DOWN: game_piece_down(player); break;
        case AI_DROP: game_piece_drop(player); break;
    }
    if(movement == AI_ROTATE || movement == AI_MOVEMENT_LEFT || movement == AI_MOVEMENT_RIGHT) {
        if(old_x == player->piece_position[0][0] && old_rotation == player->piece_rot_idx) {
            // A blocked rotation/path must not leave the computer stuck forever.
            if(++evaluation->movement_stalls >= 6) game_piece_drop(player);
        } else {
            evaluation->movement_stalls = 0;
        }
    }
    *last_tick = game_get_tick();
}

/**
 * @brief Run a match; return one when Escape requests the menu
 */
static uint8_t play_game(void) {
    uint8_t key;
    uint8_t cpu1_tick = game_get_tick();
    uint8_t cpu2_tick = cpu1_tick;
    while(game_is_running()) {
        game_update_clears();
        if(!game_is_running()) break;
        key = cbm_k_getin();
        if(key == KEYCODE_ESCAPE) {
            sound_title();
            sound_menu_effect(SFX_MENU_BACK);
            return 1;
        }
        game_parse_keyboard(key);
        if(game_mode == GAME_MODE_DEMO) {
            step_computer(&gs_player1, &pfeval_player1, &cpu1_tick);
        }
        if(game_is_running() && (game_mode == GAME_MODE_COMPUTER || game_mode == GAME_MODE_DEMO)) {
            step_computer(&gs_player2, &pfeval, &cpu2_tick);
        }
        if(game_is_running()) game_step();
    }
    return 0;
}

/**
 * @brief Advance the loss animation and wait for a return to the menu
 */
static void show_result(void) {
    uint8_t key;
    sound_result(game_mode != GAME_MODE_SINGLE && game_mode != GAME_MODE_DEMO &&
        (gs_player1.state != GAME_STATE_LOST ||
         (game_mode == GAME_MODE_LOCAL && gs_player2.state != GAME_STATE_LOST)));
    // Keep the existing fill animation, but allow immediate return to the menu.
    while(gs_player1.lose_row < 20 || gs_player2.lose_row < 20) {
        game_update_blockfill(&gs_player1);
        game_update_blockfill(&gs_player2);
        key = cbm_k_getin();
        if(key == KEYCODE_RETURN || key == KEYCODE_SPACE || key == KEYCODE_ESCAPE) return;
    }
    screen_create_lined_rectangle(10, 13, 20, 6);
    menu_centertext(14, "GAME OVER");
    if(game_mode != GAME_MODE_SINGLE) {
        if(gs_player1.state != GAME_STATE_LOST) {
            menu_centertext(16, game_mode == GAME_MODE_DEMO ? "COMPUTER 1 WINS" : "PLAYER 1 WINS");
        } else if(gs_player2.state != GAME_STATE_LOST) {
            menu_centertext(16, game_mode == GAME_MODE_DEMO ? "COMPUTER 2 WINS" :
                (game_mode == GAME_MODE_COMPUTER ? "COMPUTER WINS" : "PLAYER 2 WINS"));
        }
    }
    menu_centertext(18, "ENTER FOR MENU");
    do {
        key = cbm_k_getin();
    } while(key != KEYCODE_RETURN && key != KEYCODE_SPACE && key != KEYCODE_ESCAPE);
}

/**
 * @brief Initialize graphics and audio, then run the application scene loop
 */
void main(void) {
    uint8_t mode = GAME_MODE_COMPUTER;
    uint8_t selected;
    VERA.display.video = 0x41;
    screen_transition_init();
    sound_init();
    show_title();
    while(1) {
        show_menu();
        selected = menu_select(mode);
        if(selected == MENU_BACK) {
            show_title();
            continue;
        }
        mode = selected;
        show_game(mode);
        if(!play_game()) show_result();
    }
}
