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

#ifndef _INPUT_H
#define _INPUT_H

#include <stdint.h>

/*
 * SNES input shares the keyboard action codes. Directional holds repeat;
 * action buttons fire only on press and debounce brief releases, including
 * across scene changes.
 */

/**
 * @brief Read one controller event using the KERNAL's saved state
 *
 * @param port Controller port, one or two; excludes the keyboard joystick
 * @return Keyboard action code, or zero when no new action is available
 */
uint8_t __fastcall__ input_controller(uint8_t port);

/**
 * @brief Read keyboard input and menu actions from either controller
 *
 * @return Keyboard action code, or zero when no input is available
 */
uint8_t input_key(void);

/* Wait for Start to resume; return one if the player confirms quitting. */
uint8_t input_pause(void);
void input_freeze_begin(void);
void input_freeze_tick(void);

#endif
