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

#include "screen.h"
#include "hud_generated.h"
#include "banner_generated.h"
#include "menu_title_generated.h"
#include <stdlib.h>
#include <string.h>

/*
 * VERA tile graphics, palettes and scene setup. Positions use 16x16 tile
 * coordinates unless an API explicitly takes a pixel or VRAM address.
 */

uint8_t screen_style = SCREEN_STYLE_COMMANDER_X16;
static uint8_t menu_title_ready;
static uint8_t surface_style = 255;

/**
 * @brief Replace wallpaper tiles while preserving panel surfaces
 */
static void screen_style_background(void) {
    uint8_t x, y, low, attribute, row_tile;
    uint16_t address;
    /* Stream through two VERA ports: read each cell once, write it once.
     * Preserve grids and attributes, replacing only the wallpaper tiles. */
    for(y = 0; y < 30; y++) {
        address = (uint16_t)MAPBASE0 + ((uint16_t)y << 7);
        VERA.control = 1;
        VERA.address = address;
        VERA.address_hi = 0x11;
        VERA.control = 0;
        VERA.address = address;
        VERA.address_hi = 0x11;
        row_tile = (TILE_CYBERPUNK_FIRST & 255) + (y & 3) * 4;
        for(x = 0; x < 40; x++) {
            low = VERA.data0;
            attribute = VERA.data0;
            if((low == TILE_BG0 && (attribute & 3) == 0) ||
               ((attribute & 3) == (TILE_CYBERPUNK_FIRST >> 8) &&
                low >= (TILE_CYBERPUNK_FIRST & 255) &&
                low < (TILE_CYBERPUNK_FIRST & 255) + TILE_CYBERPUNK_COUNT)) {
                if(screen_style == SCREEN_STYLE_SYNTHWAVE) {
                    low = TILE_BG0;
                    attribute = 0;
                } else {
                    low = screen_style != SCREEN_STYLE_COMMANDER_X16 ? row_tile + (x & 3) : TILE_BG0;
                    attribute = screen_style != SCREEN_STYLE_COMMANDER_X16 ? TILE_CYBERPUNK_FIRST >> 8 : 0;
                }
            }
            VERA.data1 = low;
            VERA.data1 = attribute;
        }
    }
}

/**
 * @brief Reuse cached assets and apply the selected theme
 */
void screen_apply_style(void) {
    uint8_t video = VERA.display.video;
    static const uint8_t palettes[] = {RES_ARCADEP_DAT, RES_CYBERP_DAT, RES_FOUNDRYP_DAT, RES_SYNTHP_DAT, RES_MATRIXP_DAT};
    screen_transition_begin();
    if(surface_style != screen_style) {
        static const uint8_t surfaces[] = {RES_ARCADE_DAT, RES_TILES_DAT, RES_FOUNDRY_DAT, RES_SYNTH_DAT, RES_MATRIX_DAT};
        screen_load_tiles(palettes[screen_style], 0x1FA00UL);
        /* Foundry overrides the base frames; other styles supply a full atlas. */
        if(screen_style == SCREEN_STYLE_FOUNDRY)
            screen_load_tiles(RES_TILES_DAT, TILEBASE);
        screen_load_tiles(surfaces[screen_style], TILEBASE);
        if(screen_style != SCREEN_STYLE_SYNTHWAVE)
            screen_load_tiles(screen_style == SCREEN_STYLE_MATRIX ? RES_MATBG_DAT :
                              screen_style == SCREEN_STYLE_FOUNDRY ? RES_FORGE_DAT : RES_CYBER_DAT,
                              (uint32_t)TILE_CYBERPUNK_FIRST * 256);
        surface_style = screen_style;
        menu_title_ready = 0;
    }
    screen_style_background();
    screen_transition_end(video);
}

static uint8_t garbage_tiles_ready;
static uint8_t hud_tiles_style = 255;
static uint8_t banner_tiles_style = 255;
static uint8_t banner_spill_style = 255;
/* One digit row needs at most ten pixels; only the score has ten slots. */
static uint8_t hud_pixels[10];
static char hud_last_digits[2][75];
static uint32_t hud_last_values[2][14];
static uint8_t hud_value_ready[2][14];

/**
 * @brief Copy count glyph pixels through the selected VERA data port
 */
extern void __fastcall__ screen_hud_copy(const uint8_t* pixels, uint8_t count);

/**
 * @brief Configure VERA layers for the native 16-color splash
 */
void screen_init_title() {
    menu_title_ready = 0;
    surface_style = 255;
    garbage_tiles_ready = 0;
    hud_tiles_style = 255;
    banner_tiles_style = 255;
    banner_spill_style = 255;

    screen_loading_begin();

    // Native 640 x 480, with repeated 16x16 tiles to fit in VRAM.
    VERA.display.hscale = 128;
    VERA.display.vscale = 128;

    // layer 0
    VERA.layer0.config   = 0x12;      // 4 bpp tiles; 64x32 map
    VERA.layer0.tilebase = (TILEBASE >> 9) | 0x03; // 16x16 pixels
    VERA.layer0.mapbase  = TITLE_MAPBASE >> 9;
    VERA.layer0.hscroll  = 0;
    VERA.layer0.vscroll  = 0;

    // layer 1
    // On this layer, the values that the user fills in for the tiles are shown
    VERA.layer1.config   = (1 << 3);                        // T256C mode
    VERA.layer1.tilebase = (TILEBASE_CHARS >> 9) | 0x03;    // 16x16px tiles
    VERA.layer1.mapbase  = MAPBASE0 >> 9;                   // upper 8 bits

    // main enables layer 0 after the bitmap and palette have loaded.
}

/**
 * @brief Configure menu tile layers and prepare the selected style
 */
void screen_init_menu() {
    screen_apply_style();

    // Native 640x480 menu, using the same tiles as gameplay.
    VERA.display.hscale = 128;
    VERA.display.vscale = 128;

    // layer 0
    // This layer contains the game background and the puzzle background; this
    // includes all 'static' squares
    VERA.layer0.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer0.tilebase = (TILEBASE >> 9) | 0x03;          // 16x16px tiles
    VERA.layer0.mapbase  = MAPBASE0 >> 9;                   // upper 8 bits

    // layer 1
    // On this layer, the values that the user fills in for the tiles are shown
    VERA.layer1.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer1.tilebase = (TILEBASE >> 9) | 0x03;          // 16x16px tiles
    VERA.layer1.mapbase  = MAPBASE1 >> 9;                   // upper 8 bits

    // Keep the loading label visible while the scene is built.
    VERA.display.video = 0x41;           // VGA and loading label
}

/**
 * @brief Build two grey garbage tiles from an existing block silhouette
 *
 * Reuse the block silhouette and bevels with the existing grey palette.
 * These two tiles occupy unused space immediately after the game tileset.
 */
static void screen_create_garbage_tiles(void) {
    uint16_t pixel;
    uint8_t source, dark, light;
    for(pixel = 0; pixel < 256; pixel++) {
        source = vpeek(TILEBASE + 16UL * 256 + pixel);
        switch(source) {
            case 0: dark = light = 0; break;       // transparent corners
            case 150: dark = 15; light = 1; break; // upper bevel
            case 157: dark = 12; light = 15; break; // face
            case 156: dark = 238; light = 12; break; // lower bevel
            default: dark = light = 238; break;    // outline
        }
        vpoke(dark, TILEBASE + (uint32_t)TILE_GARBAGE_DARK * 256 + pixel);
        vpoke(light, TILEBASE + (uint32_t)TILE_GARBAGE_LIGHT * 256 + pixel);
    }
}

/**
 * @brief Configure game tile layers and prepare themed garbage blocks
 */
void screen_init_game() {
    /* Gameplay banners reuse the menu logo's VRAM atlas. */
    menu_title_ready = 0;
    screen_apply_style();
    if(!garbage_tiles_ready) {
        screen_create_garbage_tiles();
        garbage_tiles_ready = 1;
    }
    // set scaling (resolution: 640 x 480)
    VERA.display.hscale = 128;
    VERA.display.vscale = 128;

    // layer 0
    // This layer contains the game background and the puzzle background; this
    // includes all 'static' squares
    VERA.layer0.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer0.tilebase = (TILEBASE >> 9) | 0x03;          // 16x16px tiles
    VERA.layer0.mapbase  = MAPBASE0 >> 9;                   // upper 8 bits

    // layer 1
    // On this layer, the values that the user fills in for the tiles are shown
    VERA.layer1.config   = 0x03 | (1 << 4) | (1 << 6);      // 8 bpp color depth; 64 x 64 map
    VERA.layer1.tilebase = (TILEBASE >> 9) | 0x03;          // 16x16px tiles
    VERA.layer1.mapbase  = MAPBASE1 >> 9;                   // upper 8 bits

    // Keep the loading label visible while the scene is built.
    VERA.display.video = 0x41;           // VGA and loading label
}

/**
 * @brief Draw the tiled playfields for one or two players
 *
 * @param players Number of visible players, one or two
 */
void screen_create_background(uint8_t players) {
    // playfield
    screen_create_tiled_rectangle(1, 8, 10, 20);
    if(players == 2) {
        screen_create_tiled_rectangle(21, 8, 10, 20);
    }

}

/**
 * @brief Load the HUD atlas and draw the active players' score cards
 *
 * The static HUD uses deduplicated tiles; numeric tiles belong to one
 * field.
 *
 * @param players Number of visible players, one or two
 */
void screen_create_scorebox(uint8_t players) {
    uint8_t row, col, limit = players == 1 ? 20 : 40;
    uint16_t tile, address;
    if(hud_tiles_style != screen_style) {
        static const uint8_t atlases[] = {RES_HUD_A_DAT, RES_HUD_DAT, RES_HUD_F_DAT, RES_HUD_S_DAT, RES_HUD_M_DAT};
        screen_load_tiles(atlases[screen_style], 130UL * 256);
        memset(hud_last_digits, 0, sizeof(hud_last_digits));
        memset(hud_value_ready, 0, sizeof(hud_value_ready));
        hud_tiles_style = screen_style;
    }
    for(row = 0; row < 30; row++) {
        if(!hud_rows[row]) continue;
        for(col = 0; col < limit; col++) {
            tile = hud_map[hud_rows[row] - 1][col];
            if(!tile) continue;
            tile += 129;
            address = (uint16_t)MAPBASE1 + ((uint16_t)row << 7) + ((uint16_t)col << 1);
            VERA.address = address;
            VERA.address_hi = 0x11;
            VERA.data0 = tile;
            VERA.data0 = tile >> 8;
        }
    }
}

/**
 * @brief Copy one glyph row at the HUD field's pixel scale
 *
 * Decode packed glyph bits; double pixels only when drawing the large score.
 */
static void screen_hud_digit_row(uint8_t* pixels, const uint8_t* glyph,
                                 uint8_t row, uint8_t scale) {
    uint8_t col, color, bits = glyph[scale == 1 ? row : row >> 1];
    for(col = 0; col < 5; col++) {
        color = bits & 16 ? 11 : 30;
        bits <<= 1;
        *pixels++ = color;
        if(scale == 2) *pixels++ = color;
    }
}

/**
 * @brief Update changed digits in one player's numeric HUD field
 *
 * Numeric fields update only changed glyphs; all other pixels stay cached.
 *
 * @param player PLAYER1 or PLAYER2
 * @param field  HUD_* field index
 * @param value  Unsigned value to display
 */
void screen_draw_hud_number(uint8_t player, uint8_t field, uint32_t value) {
    const struct HudField* f = &hud_fields[field];
    char text[11], ch;
    char* cached = hud_last_digits[player] + (field ? 5 + field * 5 : 0);
    uint8_t length, slots, slot, row, scale = f->scale, width;
    uint8_t remaining, chunk, written;
    uint16_t x, y, tile;
    const uint8_t* glyph;
    if(hud_value_ready[player][field] && hud_last_values[player][field] == value) return;
    if(field == HUD_SCORE) ultoa(value, text, 10);
    else utoa((uint16_t)value, text, 10);
    length = strlen(text);
    slots = ((f->width / scale) + 1) / 6;
    width = 5 * scale;
    for(slot = 0; slot < slots; slot++) {
        ch = slot < slots - length ? ' ' : text[slot - (slots - length)];
        if(cached[slot] == ch ||
           (!cached[slot] && ch == ' ')) continue;
        if(ch != ' ') glyph = hud_digits[ch - '0'];
        for(row = 0; row < f->height; row++) {
            if(ch == ' ') memset(hud_pixels, f->background, width);
            else screen_hud_digit_row(hud_pixels, glyph, row, scale);
            x = f->x + (uint16_t)player * 320 + slot * 6 * scale;
            y = f->y + row;
            remaining = width;
            written = 0;
            while(remaining) {
                tile = 129 + (uint16_t)hud_map[hud_rows[y >> 4] - 1][x >> 4];
                VERA.address = (tile << 8) + ((y & 15) << 4) + (x & 15);
                VERA.address_hi = 0x10 | (tile >> 8);
                chunk = 16 - (x & 15);
                if(chunk > remaining) chunk = remaining;
                remaining -= chunk;
                x += chunk;
                screen_hud_copy(hud_pixels + written, chunk);
                written += chunk;
            }
        }
        cached[slot] = ch;
    }
    hud_last_values[player][field] = value;
    hud_value_ready[player][field] = 1;
}

/**
 * @brief Load the theme's primary banner atlas, which the menu logo spares
 */
static void screen_load_banner_tiles(void) {
    static const uint8_t atlases[] = {RES_NAME_L_DAT, RES_NAME_C_DAT, RES_NAME_F_DAT, RES_NAME_S_DAT, RES_NAME_M_DAT};
    if(banner_tiles_style == screen_style) return;
    screen_load_tiles(atlases[screen_style], (uint32_t)BANNER_TILE_FIRST * 256);
    banner_tiles_style = screen_style;
}

/**
 * @brief Draw the themed computer face from the banner atlas
 *
 * @param x Horizontal tile coordinate of the 2x2 face
 * @param y Vertical tile coordinate of the 2x2 face
 */
void screen_draw_computer_face(uint8_t x, uint8_t y) {
    uint8_t tile;
    screen_load_banner_tiles();
    for(tile = 0; tile < 4; tile++) {
        if(!(tile & 1)) {
            VERA.address = (uint16_t)MAPBASE1 + ((uint16_t)y++ << 7) + (x << 1);
            VERA.address_hi = 0x11;
        }
        VERA.data0 = (BANNER_TILE_FIRST & 255) + tile;
        VERA.data0 = BANNER_TILE_FIRST >> 8;
    }
}

/**
 * @brief Draw the themed player or computer nameplate
 *
 * Banners reuse a theme atlas; all lettering and icons are already baked.
 *
 * @param player   PLAYER1 or PLAYER2
 * @param computer Nonzero for a computer nameplate; zero for a player nameplate
 * @param number   Displayed player or computer number, one or two
 */
void screen_draw_player_banner(uint8_t player, uint8_t computer, uint8_t number) {
    uint8_t row, col, variant = (computer ? 2 : 0) + (number == 2 ? 1 : 0);
    uint16_t address;
    const uint8_t* tiles;
    screen_load_banner_tiles();
    if(banner_spill_style != screen_style) {
        static const uint8_t spills[] = {RES_NAME_LX_DAT, RES_NAME_CX_DAT, RES_NAME_FX_DAT, RES_NAME_SX_DAT, RES_NAME_MX_DAT};
        screen_load_tiles(spills[screen_style],
                          (uint32_t)BANNER_TILE_SPILL * 256);
        banner_spill_style = screen_style;
    }
    for(row = 0; row < 2; row++) {
        address = (uint16_t)MAPBASE1 + ((uint16_t)row << 7) + ((1 + player * 20) << 1);
        tiles = banner_map[screen_style][variant][row];
        VERA.address = address;
        VERA.address_hi = 0x11;
        for(col = 0; col < 18; col++) {
            VERA.data0 = tiles[col];
            VERA.data0 = 1;
        }
    }
}

/**
 * @brief Draw the next-piece boxes and their labels
 *
 * @param players Number of visible players, one or two
 */
void screen_draw_next_boxes(uint8_t players) {
    uint8_t i;
    const char next_str[] = "NEXT";

    // next piece box
    screen_create_tiled_rectangle(14, 10, 4, 3);
    if(players == 2) screen_create_tiled_rectangle(34, 10, 4, 3);

    // text
    for(i=0; i<4; i++) {
        screen_set_tile_layer1(14+i, 8, next_str[i] - 'A' + 96);
        if(players == 2) screen_set_tile_layer1(34+i, 8, next_str[i] - 'A' + 96);
    }
}

/**
 * @brief Clear foreground tiles inside a panel and redraw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_clear_rectangle(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i,j;
    uint8_t ctr = 0;
    uint32_t map_base_addr;

    for(i=0; i<h; i++) {
        map_base_addr = MAPBASE1 + ((y + i) << 7) + (x << 1);
        VERA.address = map_base_addr;
        VERA.address_hi = map_base_addr >> 16;
        VERA.address_hi |= (1 << 4);
        for(j=0; j<w; j++) {
            VERA.data0 = TILE_NONE;
            VERA.data0 = 0;
        }
        ctr++;
    }

    // also create border
    screen_create_border(x, y, w, h);
}

/**
 * @brief Fill a checkerboard panel and draw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
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
 * @brief Fill a lined panel, clear its foreground and draw its border
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
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
    screen_clear_rectangle(x, y, w, h);
}

/**
 * @brief Draw a border immediately outside a rectangle
 *
 * @param x Horizontal tile coordinate
 * @param y Vertical tile coordinate
 * @param w Interior width in tiles
 * @param h Interior height in tiles
 */
void screen_create_border(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
    uint8_t i;
    screen_set_tile_layer1(x - 1, y - 1, TILE_BORDER_LT);
    screen_set_tile_layer1(x + w, y - 1, TILE_BORDER_RT);
    screen_set_tile_layer1(x - 1, y + h, TILE_BORDER_LB);
    screen_set_tile_layer1(x + w, y + h, TILE_BORDER_RB);
    for(i = 0; i < w; i++) {
        screen_set_tile_layer1(x + i, y - 1, TILE_BORDER_TOP);
        screen_set_tile_layer1(x + i, y + h, TILE_BORDER_BOTTOM);
    }
    for(i = 0; i < h; i++) {
        screen_set_tile_layer1(x - 1, y + i, TILE_BORDER_LEFT);
        screen_set_tile_layer1(x + w, y + i, TILE_BORDER_RIGHT);
    }
}

/**
 * @brief Set a foreground tile and clear its extended tile attributes
 *
 * @param x    Horizontal tile coordinate
 * @param y    Vertical tile coordinate
 * @param tile Base tile index
 */
void screen_set_tile_layer1(uint8_t x, uint8_t y, uint8_t tile) {
    uint16_t address = (uint16_t)MAPBASE1 + ((uint16_t)y << 7) + ((uint16_t)x << 1);
    VERA.address = address;
    VERA.address_hi = 0x11; // Fixed VRAM bank, increment by one byte.
    VERA.data0 = tile;
    VERA.data0 = 0; // Clear extended HUD tile bits when drawing ordinary tiles.
}

/**
 * @brief Write a tile and its attribute byte to the background layer
 *
 * @param x   Horizontal tile coordinate
 * @param y   Vertical tile coordinate
 * @param val Tile byte to write
 * @param col Tile attribute byte
 */
void screen_draw_character(uint8_t x, uint8_t y, uint8_t val, uint8_t col) {
    uint32_t map_base_addr = MAPBASE0 + (y << 6) + (x << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= 0b10000;
    VERA.data0 = val;
    VERA.data0 = col;
}

/**
 * @brief Redraw the twenty visible rows from board occupancy
 *
 * @param x         Horizontal tile coordinate of the playfield's left edge
 * @param playfield Row-major board cells, starting at the bottom row; 0xFF is empty
 */
void screen_redraw_playfield(uint8_t x, uint8_t* playfield) {
    uint8_t row, col, type;
    uint16_t address = (uint16_t)MAPBASE1 + (27U << 7) + ((uint16_t)x << 1);
    for(row = 0; row < 20; row++) {
        VERA.address = address;
        VERA.address_hi = 0x21; // Increment by two, leaving tile attributes alone.
        for(col = 0; col < PLAYFIELD_WIDTH; col++) {
            type = *playfield++;
            VERA.data0 = type == 0xFF ? TILE_NONE : type + 16;
        }
        address -= 128;
    }
}

/**
 * @brief Erase one visible playfield cell during a row wipe
 *
 * @param xoffset Horizontal tile coordinate of the playfield's left edge
 * @param row     Board row, 0..19, counted upward from the bottom
 * @param column  Board column, 0..9
 */
void screen_clear_row_cell(uint8_t xoffset, uint8_t row, uint8_t column) {
    screen_set_tile_layer1(xoffset + column, 27 - row, TILE_NONE);
}

/**
 * @brief Draw the themed splash lettering above the main menu
 */
void screen_draw_menu_title(void) {
    uint8_t row, column;
    uint8_t tile;
    if(!menu_title_ready) {
        screen_load_tiles(screen_style == SCREEN_STYLE_COMMANDER_X16 ? RES_MENU_A_DAT : RES_MENU_DAT,
                          (uint32_t)MENU_TITLE_FIRST * 256);
        /* The logo overwrites the spill tiles of cached gameplay banners. */
        banner_spill_style = 255;
        menu_title_ready = 1;
    }
    for(row = 0; row < 2; row++) {
        VERA.address = (uint16_t)MAPBASE1 + ((uint16_t)(row + 1) << 7) + 20;
        VERA.address_hi = 0x11;
        for(column = 0; column < 20; column++) {
            tile = menu_title_map[row][column];
            VERA.data0 = tile;
            VERA.data0 = tile ? MENU_TITLE_FIRST >> 8 : 0;
        }
    }
}
