/**************************************************************************
 *                                                                        *
 *   Author: Ivo Filot <ivo@ivofilot.nl>                                  *
 *                                                                        *
 *   CX16-KAKURO is free software:                                        *
 *   you can redistribute it and/or modify it under the terms of the      *
 *   GNU General Public License as published by the Free Software         *
 *   Foundation, either version 3 of the License, or (at your option)     *
 *   any later version.                                                   *
 *                                                                        *
 *   CX16-KAKURO is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty          *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.              *
 *   See the GNU General Public License for more details.                 *
 *                                                                        *
 *   You should have received a copy of the GNU General Public License    *
 *   along with this program.  If not, see http://www.gnu.org/licenses/.  *
 *                                                                        *
 **************************************************************************/

#ifndef _CONSTANTS_H
#define _CONSTANTS_H

// graphics positioning in VRAM
#define TILEBASE            0x00000      // 256 * 16 tiles at 8bpp
#define MAPBASE0            0x18000      // 64 x 64 tiles
#define MAPBASE1            0x1A000      // 64 x 64 tiles
#define PALETTEBASE         0x1FA20
#define PALETTEBYTE         0x00
#define BANKED_RAM          0xA000

#define MAPHEIGHT           64
#define MAPWIDTH            64

#define LAYER0              0
#define LAYER1              1

#define TILE_NONE           0x00
#define TILE_BG0            0x01
#define TILE_BG1            0x03
#define TILE_BG2            0x04

#define TILE_BORDER_LT      0x05
#define TILE_BORDER_RT      0x06
#define TILE_BORDER_LB      0x07
#define TILE_BORDER_RB      0x08

#define TILE_BORDER_TOP      0x09
#define TILE_BORDER_RIGHT    0x0A
#define TILE_BORDER_LEFT     0x0B
#define TILE_BORDER_BOTTOM   0x0C

#define TILE_ENDGAME         0x2B

#define PLAYER1              0x00
#define PLAYER2              0x01

#define KEYCODE_DOWN        0x11
#define KEYCODE_UP          0x91
#define KEYCODE_LEFT        0x9D
#define KEYCODE_RIGHT       0x1D
#define KEYCODE_RETURN      0x0D
#define KEYCODE_SPACE       0x20
#define KEYCODE_ESCAPE      0x1B
#define KEYCODE_0           0x30

#define TILE_SHADOW         23

#define MOVE_RIGHT          1
#define MOVE_LEFT           -1

#define GAME_STATE_RESET    0x00
#define GAME_STATE_RUN      0x01
#define GAME_STATE_LOST     0x02
#define GAME_STATE_PAUSE    0x03

#define AI_NONE             0x00
#define AI_MOVEMENT_LEFT    0x01
#define AI_MOVEMENT_RIGHT   0x02
#define AI_MOVEMENT_DOWN    0x03
#define AI_ROTATE           0x04
#define AI_DROP             0x05

#define PLAYFIELD_WIDTH     10
#define PLAYFIELD_HEIGHT    24

#endif // _CONSTANTS_H
