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

/**
 * @brief Initialize screen
 * 
 */
void screen_init() {
    // set scaling (resolution: 640 x 480)
    VERA.display.hscale = 128;
    VERA.display.vscale = 128;

    // layer 0
    // This layer contains the game background and the puzzle background; this
    // includes all 'static' squares
    VERA.layer0.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer0.tilebase = (TILEBASE >> 9) | 0x03;          // 16 x 16 tiles
    VERA.layer0.mapbase  = MAPBASE0 >> 9;                   // upper 8 bits
    
    // layer 1
    // On this layer, the values that the user fills in for the tiles are shown
    VERA.layer1.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer1.tilebase = (TILEBASE >> 9) | 0x03;          // 16 x 16 tiles
    VERA.layer1.mapbase  = MAPBASE1 >> 9;                   // upper 8 bits

    // enable both layers, no sprites
    //VERA.display.video |= 0b00110000;
    VERA.display.video = (1 << 5) | (1 << 4) | 1;           // VGA and layers 1 and 2

    // enable sprites
    //VERA.display.video |= 0b01000000;
}

/**
 * @brief Load the tiles from file into video memory
 * 
 */
void screen_load_tiles(const char* filename, uint32_t addr) {
    cbm_k_setnam(filename);
    cbm_k_setlfs(0, 8, 2);
    cbm_k_load(2 + (addr >> 16), addr); // will load directly into VRAM
}