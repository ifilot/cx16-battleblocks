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

/**
 * @brief Initialize screen
 * 
 */
void screen_init();

/**
 * @brief Load the tiles from file into memory
 * 
 */
void screen_load_tiles(const char* filename, uint32_t addr);

/**
 * @brief Create the scene background
 * 
 */
void screen_create_background();

/**
 * @brief Fill out the scorebox
 * 
 */
void screen_create_scorebox();

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

/**
 * @brief Redraw the playing field based on array
 * 
 * @param x 
 * @param playfield 
 */
void screen_redraw_playfield(uint8_t x, uint8_t* playfield);

#endif // _VIDEO_H