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

#ifndef _VIDEO_H
#define _VIDEO_H

#include <cx16.h>
#include <cbm.h>
#include <ascii_charmap.h>
#include <stdint.h>

#include "constants.h"

/*
 * VERA tile graphics, palettes and scene setup. Positions use 16x16 tile
 * coordinates unless an API explicitly takes a pixel or VRAM address.
 */

#define SCREEN_STYLE_COMMANDER_X16 0
#define SCREEN_STYLE_CYBERPUNK 1
#define SCREEN_STYLE_FOUNDRY 2
#define SCREEN_STYLE_SYNTHWAVE 3
#define SCREEN_STYLE_MATRIX 4
#define SCREEN_STYLE_COUNT 5
extern uint8_t screen_style; // One of the SCREEN_STYLE_* values

/**
 * @brief Draw the themed splash lettering above the main menu
 */
void screen_draw_menu_title(void);

/**
 * @brief Reuse cached assets and apply the selected theme
 */
void screen_apply_style(void);

/**
 * @brief Configure VERA layers for the native 16-color splash
 */
void screen_init_title();

/**
 * @brief Configure menu tile layers and prepare the selected style
 */
void screen_init_menu();

/**
 * @brief Configure game tile layers and prepare themed garbage blocks
 */
void screen_init_game();

/**
 * @brief Stream a bundled graphics resource directly into VRAM
 *
 * @param resource Generated graphics resource ID
 * @param addr     Destination VRAM byte address
 */
void screen_load_tiles(uint8_t resource, uint32_t addr);

#include "resources_generated.h"

/**
 * @brief Draw the tiled playfields for one or two players
 *
 * @param players Number of visible players, one or two
 */
void screen_create_background(uint8_t players);

/**
 * @brief Load the HUD atlas and draw the active players' score cards
 *
 * @param players Number of visible players, one or two
 */
void screen_create_scorebox(uint8_t players);

/**
 * @brief Draw the next-piece boxes and their labels
 *
 * @param players Number of visible players, one or two
 */
void screen_draw_next_boxes(uint8_t players);

/**
 * @brief Update changed digits in one player's numeric HUD field
 *
 * @param player PLAYER1 or PLAYER2
 * @param field  HUD_* field index
 * @param value  Unsigned value to display
 */
void screen_draw_hud_number(uint8_t player, uint8_t field, uint32_t value);

/**
 * @brief Draw the themed computer face from the banner atlas
 *
 * @param x Horizontal tile coordinate of the 2x2 face
 * @param y Vertical tile coordinate of the 2x2 face
 */
void screen_draw_computer_face(uint8_t x, uint8_t y);

/**
 * @brief Draw the themed player or computer nameplate
 *
 * @param player   PLAYER1 or PLAYER2
 * @param computer Nonzero for a computer nameplate; zero for a player nameplate
 * @param number   Displayed player or computer number, one or two
 */
void screen_draw_player_banner(uint8_t player, uint8_t computer, uint8_t number);

/**
 * @brief Fill a checkerboard panel and draw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_create_tiled_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Fill a lined panel, clear its foreground and draw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_create_lined_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Clear foreground tiles inside a panel and redraw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_clear_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Draw a border immediately outside a rectangle
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_create_border(uint8_t x, uint8_t y, uint8_t w, uint8_t h);

/**
 * @brief Fill the entire 64-by-64 tilemap layer with one tile
 *
 * @param layer   LAYER0 or LAYER1
 * @param tile_id Tile index used for every cell
 */
extern void __fastcall__ screen_fill_layer(uint8_t layer, uint8_t tile_id);

/**
 * @brief Set a foreground tile and clear its extended tile attributes
 *
 * @param x    Horizontal tile coordinate
 * @param y    Vertical tile coordinate
 * @param tile Base tile index
 */
void screen_set_tile_layer1(uint8_t x, uint8_t y, uint8_t tile);

/**
 * @brief Write a tile and its attribute byte to the background layer
 *
 * @param x   Horizontal tile coordinate
 * @param y   Vertical tile coordinate
 * @param val Tile byte to write
 * @param col Tile attribute byte
 */
void screen_draw_character(uint8_t x, uint8_t y, uint8_t val, uint8_t col);

/**
 * @brief Redraw the twenty visible rows from board occupancy
 *
 * @param x         Horizontal tile coordinate of the playfield's left edge
 * @param playfield Row-major board cells, starting at the bottom row; 0xFF is empty
 */
void screen_redraw_playfield(uint8_t x, uint8_t* playfield);

/**
 * @brief Erase one visible playfield cell during a row wipe
 *
 * Erase one cell of a completed row inside the playfield.
 *
 * @param xoffset Horizontal tile coordinate of the playfield's left edge
 * @param row     Board row, 0..19, counted upward from the bottom
 * @param column  Board column, 0..9
 */
void screen_clear_row_cell(uint8_t xoffset, uint8_t row, uint8_t column);

/**
 * @brief Prepare the resident loading label and initial loading screen
 *
 * Resident themed loading label; show each scene as soon as it is ready.
 */
void screen_transition_init(void);

/**
 * @brief Show the themed loading label on a plain background
 */
void screen_loading_begin(void);

/**
 * @brief Hide scene layers while the next scene is being prepared
 */
void screen_transition_begin(void);

/**
 * @brief Restore the requested VERA display-enable bits
 *
 * @param video VERA display control byte to restore
 */
void __fastcall__ screen_transition_end(uint8_t video);

#endif // _VIDEO_H
