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

/**
 * @brief Create the scene background
 * 
 */
void screen_create_background() {
    // playfield
    screen_create_tiled_rectangle(2, 8, 10, 20);
    screen_create_tiled_rectangle(22, 8, 10, 20);

    //score boxes
    screen_create_lined_rectangle(2, 2, 16, 3);
    screen_create_lined_rectangle(22, 2, 16, 3);
}

/**
 * @brief Fill out the scorebox
 * 
 */
void screen_create_scorebox() {
    uint8_t i;
    const char level_str[5] = "LEVEL";
    const char score_str[5] = "SCORE";
    const char lines_str[5] = "LINES";

    // level
    for(i=0; i<5; i++) {
        screen_set_tile_layer1(2+i, 2, level_str[i] - 'A' + 96);
        screen_set_tile_layer1(22+i, 2, level_str[i] - 'A' + 96);
    }

    // score
    for(i=0; i<5; i++) {
        screen_set_tile_layer1(2+i, 3,  score_str[i] - 'A' + 96);
        screen_set_tile_layer1(22+i, 3, score_str[i] - 'A' + 96);
    }

    // lines
    for(i=0; i<5; i++) {
        screen_set_tile_layer1(2+i, 4, lines_str[i] - 'A' + 96);
        screen_set_tile_layer1(22+i, 4, lines_str[i] - 'A' + 96);
    }
}

/**
 * @brief Draw "next piece" boxes
 * 
 */
void screen_draw_next_boxes() {
    uint8_t i;
    const char next_str[] = "NEXT";

    // next piece box
    screen_create_tiled_rectangle(14, 10, 4, 3);
    screen_create_tiled_rectangle(34, 10, 4, 3);

    // text
    for(i=0; i<4; i++) {
        screen_set_tile_layer1(14+i, 8, next_str[i] - 'A' + 96);
        screen_set_tile_layer1(34+i, 8, next_str[i] - 'A' + 96);
    }
}

/**
 * @brief Clear rectangular area on layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_clear_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i,j;
    uint8_t ctr = 0;
    uint32_t map_base_addr;

    for(i=0; i<h; i++) {
        map_base_addr = MAPBASE1 + ((y + i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= (1 << 5);
        for(j=0; j<w; j++) {
            VERA.data0 = TILE_NONE;
        }
        ctr++;
    }

    // also create border
    screen_create_border(x, y, w, h);
}

/**
 * @brief Created tiles background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_tiled_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i,j;
    uint8_t ctr = 0;
    uint32_t map_base_addr;

    for(i=0; i<h; i++) {
        map_base_addr = MAPBASE0 + ((y + i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= 0b10000;
        for(j=0; j<w; j++) {
            VERA.data0 = (ctr++ % 2 == 0) ? TILE_BG1 : TILE_BG2;
            VERA.data0 = 0;
        }
        ctr++;
    }

    // also create border
    screen_create_border(x, y, w, h);
}

/**
 * @brief Created tiles background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_lined_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i,j;
    uint8_t ctr = 0;
    uint32_t map_base_addr;

    for(i=0; i<h; i++) {
        map_base_addr = MAPBASE0 + ((y + i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= 0b10000;
        for(j=0; j<w; j++) {
            VERA.data0 = (ctr % 2 == 0) ? TILE_BG1 : TILE_BG2;
            VERA.data0 = 0;
        }
        ctr++;
    }

    // also create border
    screen_create_border(x, y, w, h);
}

/**
 * @brief Created lined background
 * 
 * @param x x-position
 * @param y y-position
 * @param w number of tiles width
 * @param h number of tiles height
 */
void screen_create_border(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i;
    uint8_t ctr = 0;
    uint32_t map_base_addr;

    // top
    map_base_addr = MAPBASE1 + ((y - 1) << 7) + ((x-1) << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (2 << 4);
    VERA.data0 = TILE_BORDER_LT;
    for(i=0; i<w; i++) {
        VERA.data0 = TILE_BORDER_TOP;
    }
    VERA.data0 = TILE_BORDER_RT;

    // bottom
    map_base_addr = MAPBASE1 + ((y + h) << 7) + ((x-1) << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (2 << 4);
    VERA.data0 = TILE_BORDER_LB;
    for(i=0; i<w; i++) {
        VERA.data0 = TILE_BORDER_BOTTOM;
    }
    VERA.data0 = TILE_BORDER_RB;

    // left
    map_base_addr = MAPBASE1 + (y << 7) + ((x-1) << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (8 << 4);
    for(i=0; i<h; i++) {
        VERA.data0 = TILE_BORDER_LEFT;
    }

    // right
    map_base_addr = MAPBASE1 + (y << 7) + ((x+w) << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (8 << 4);
    for(i=0; i<h; i++) {
        VERA.data0 = TILE_BORDER_RIGHT;
    }
}

/**
 * @brief Position a single tile on the screen for layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param tile tile_id
 */
void screen_set_tile_layer1(uint8_t x, uint8_t y, uint8_t tile) {
    uint32_t map_base_addr = MAPBASE1 + (y << 7) + (x << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= 0b10000;
    VERA.data0 = tile;
}

/**
 * @brief Redraw the playing field based on array
 * 
 * @param x 
 * @param playfield 
 */
void screen_redraw_playfield(uint8_t x, uint8_t* playfield) {
    uint8_t i,j;
    uint32_t map_base_addr;
    uint8_t tile_id;

    for(i=0; i<20; i++) {
        map_base_addr= MAPBASE1 + (8 + (19-i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= (1 << 5);    // increment by 2
        
        for(j=0; j<10; j++) {
            tile_id = *(playfield++);
            VERA.data0 = (tile_id != 0xFF) ? (tile_id + 16) : TILE_NONE;
        }
    }
}