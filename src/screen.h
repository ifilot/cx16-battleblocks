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

#ifndef _VIDEO_H
#define _VIDEO_H

#include <cx16.h>
#include <cbm.h>
#include <ascii_charmap.h>
#include <stdint.h>

#include "constants.h"

#define SCREEN_STYLE_CLASSIC 0
#define SCREEN_STYLE_CYBERPUNK 1
extern uint8_t screen_style;
void screen_apply_style(void);
void screen_apply_title_style(void);

/**
 * @brief Initialize screen
 * 
 */
void screen_init_title();

/**
 * @brief Initialize screen
 * 
 */
void screen_init_menu();

/**
 * @brief Initialize screen
 * 
 */
void screen_init_game();

/**
 * @brief Load the tiles from file into memory
 * 
 */
void screen_load_tiles(const char* filename, uint32_t addr);

/**
 * @brief Create the scene background
 * 
 */
void screen_create_background(uint8_t players);

/**
 * @brief Fill out the scorebox
 * 
 */
void screen_create_scorebox(uint8_t players);

/**
 * @brief Draw "next piece" boxes
 * 
 */
void screen_draw_next_boxes(uint8_t players);
void screen_draw_hud_number(uint8_t player, uint8_t field, uint32_t value);
void screen_draw_player_banner(uint8_t player, uint8_t computer, uint8_t number);

/**
 * @brief Created tiled background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_tiled_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Created lined background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_lined_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Clear rectangular area on layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_clear_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Created lined background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_border(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Fill the specified tilemap layer with the same tile.
 *
 * @param layer Layer index (0 or 1)
 * @param tile_id Tile index to fill the layer with
 */
extern void __fastcall__ screen_fill_layer(uint8_t layer, uint8_t tile_id);

/**
 * @brief Position a single tile on the screen for layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param tile tile_id
 */
void screen_set_tile_layer1(uint8_t x, uint8_t y, uint8_t tile);

void screen_draw_character(uint8_t x, uint8_t y, uint8_t val, uint8_t col);

/**
 * @brief Redraw the playing field based on array
 * 
 * @param x 
 * @param playfield 
 */
void screen_redraw_playfield(uint8_t x, uint8_t* playfield);

/* Erase one cell of a completed row inside the playfield. */
void screen_clear_row_cell(uint8_t xoffset, uint8_t row, uint8_t column);

/* Resident themed loading label; show each scene as soon as it is ready. */
void screen_transition_init(void);
void screen_loading_begin(void);
void screen_transition_begin(void);
void __fastcall__ screen_transition_end(uint8_t video);

#endif // _VIDEO_H
