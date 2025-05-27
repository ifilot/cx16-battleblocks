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

#endif // _CONSTANTS_H
