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

#include "screen.h"
#include "game.h"

void main() {
    static uint8_t keycode;
    uint8_t start, now;

    // initialize screen
    screen_init();

    // load tile data
    screen_load_tiles("tiles.dat", TILEBASE);

    // clear screen
    screen_fill_layer(LAYER0, 0x01);
    screen_fill_layer(LAYER1, 0x00);

    screen_create_background();

    game_init();

    while(1) {
        start = game_get_tick();
        now = start;

        while((now - start) < 49) {
            asm("jsr $FFE4");
            asm("sta %v", keycode);
            game_parse_keyboard(keycode);
            now = game_get_tick();
        }

        game_step();
    }
 }