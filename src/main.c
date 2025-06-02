/**************************************************************************
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   CX16-BATTLE-BLOCKS is free software:                                 *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   CX16-BATTLE-BLOCKS is distributed in the hope that it will be useful,*
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#include <stdio.h>
#include "screen.h"
#include "game.h"
#include "ai.h"

void main() {
    static uint8_t keycode;
    uint8_t start, now;
    uint8_t startcpu;
    uint8_t mov;

    // initialize screen
    screen_init();

    // load tile data
    screen_load_tiles("tiles.dat", TILEBASE);

    // clear screen
    screen_fill_layer(LAYER0, 0x01);
    screen_fill_layer(LAYER1, 0x00);

    screen_create_background();
    screen_create_scorebox();

    game_init();

    printf("\n");

    startcpu = game_get_tick();

    while(game_is_running()) {
        start = game_get_tick();
        now = start;

        while((now - start) < 49) {
            asm("jsr $FFE4");
            asm("sta %v", keycode);
            game_parse_keyboard(keycode);

            if(ai_calculate_next_score(&gs_player2, &pfeval) == 1) {
                if((now - startcpu) > 1) {
                    mov = ai_determine_movement(&gs_player2, &pfeval);
                    switch(mov) {
                        case AI_MOVEMENT_LEFT:
                            game_piece_move(&gs_player2, -1);
                        break;
                        case AI_MOVEMENT_RIGHT:
                            game_piece_move(&gs_player2, 1);
                        break;
                        case AI_ROTATE:
                            game_piece_rotate(&gs_player2);
                        break;
                        case AI_MOVEMENT_DOWN:
                            game_piece_down(&gs_player2);
                        break;
                        case AI_DROP:
                            game_piece_drop(&gs_player2);
                        break;
                    }
                    startcpu = game_get_tick();
                }
            }

            now = game_get_tick();
        }

        game_step();
    }
 }