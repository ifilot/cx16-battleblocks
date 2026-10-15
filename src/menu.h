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

#ifndef _MENU_H
#define _MENU_H

#include <cx16.h>
#include <cbm.h>
#include <ascii_charmap.h>
#include <stdint.h>
#include "constants.h"

/*
 * Tile-font menus, theme selection, audio options and application details.
 * The selected game mode is returned to the application scene loop.
 */

/**
 * @brief Draw text in the menu font, clipped to the 40-by-30 viewport
 *
 * @param x   Horizontal tile coordinate
 * @param y   Vertical tile coordinate
 * @param str Null-terminated text; letters are converted to uppercase
 */
void menu_settext(uint8_t x, uint8_t y, const char* str);

/**
 * @brief Center menu text horizontally on the requested row
 *
 * @param y   Vertical tile coordinate
 * @param str Null-terminated text; letters are converted to uppercase
 */
void menu_centertext(uint8_t y, const char* str);

/* Main menu entry that leaves the game; menu_select returns it after Yes. */
#define MENU_EXIT 7

/*
 * Ask a yes/no question in a modal box with No preselected, preserving the
 * display and match time on cancellation. Return one for Yes.
 */
uint8_t __fastcall__ menu_confirm(const char* title);

/* Confirm leaving a match. */
uint8_t menu_quit(void);

/**
 * @brief Run the main menu and return the selected game mode
 *
 * @param initial_mode Zero-based initial menu selection, 0..7
 *
 * @return Selected GAME_MODE_* value, or MENU_EXIT
 */
uint8_t menu_select(uint8_t initial_mode);

#endif // _MENU_H
