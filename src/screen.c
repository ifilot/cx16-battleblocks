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
    VERA.layer0.config   = (1 << 3) | (1 << 4) | (1 << 6);      // 8 bpp color depth
    VERA.layer0.tilebase = (TILEBASE >> 9) | 0x03;              // 16 x 16 tiles
    VERA.layer0.mapbase  = MAPBASE0 >> 9;                       // upper 8 bits
    
    // layer 1
    // On this layer, the values that the user fills in for the tiles are shown
    VERA.layer1.config   = (1 << 3) | (1 << 4) | (1 << 6);      // 8 bpp color depth
    VERA.layer1.tilebase = (TILEBASE >> 9) | 0x03;              // 16 x 16 tiles
    VERA.layer1.mapbase  = MAPBASE1 >> 9;

    // enable both layers
    VERA.display.video |= 0b00110000;

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
    cbm_k_load(2 + (addr >> 16), addr);
}

/**
 * @brief Fill layer 0 with all tiles of the same type
 *
 * @param tile_id background tile index
 * @param layer   which layer to fill
 * @param b2      which tile info to use
 * @param height  height of the map
 * @param width   width of the map
 */
void screen_fill_layer(uint8_t tile_id, uint8_t layer, uint8_t b2, uint8_t height, uint8_t width) {
    uint8_t i,j;
    uint32_t map_base_addr;

    // set background tiles
    map_base_addr = (layer == 0 ? MAPBASE0 : MAPBASE1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= 0b10000;

    for (j=0; j<height; j++) {
        for (i=0; i<width; i++) {
            VERA.data0 = tile_id;       // background tile
            VERA.data0 = b2;            // palette offset data
        }
    }
}