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
#include "hud_generated.h"
#include "banner_generated.h"
#include <stdlib.h>
#include <string.h>

uint8_t screen_style = SCREEN_STYLE_CLASSIC;
static uint8_t original_palette[512];
static uint8_t palette_saved;

/* VERA colors are 12-bit RGB. Only scene transitions/menu input touch these. */
struct PaletteColor { uint8_t index; uint16_t rgb; };
static const struct PaletteColor cyberpunk_palette[] = {
    {1, 0xDFF}, {11, 0x8EF}, {16, 0x8EF}, {18, 0x012},
    {30, 0x012}, {147, 0x046}, {240, 0xDFF},
    {232, 0x145}, {233, 0x123}, {234, 0x012}, {235, 0x012},
    {236, 0x012}, {237, 0x123}, {239, 0x001},
    /* Circuit wallpaper: navy, etched tracks, contacts, muted pink accent. */
    {242, 0x012}, {243, 0x123}, {244, 0x145}, {245, 0x624},
    /* Keep seven recognizable piece colors and their bevels. */
    {167, 0x056}, {150, 0xBFF}, {157, 0x0EF}, {156, 0x09B},
    {84, 0x650}, {73, 0xFFA}, {87, 0xFE2}, {86, 0xBA0},
    {223, 0x306}, {220, 0xD8F}, {227, 0xA3F}, {225, 0x71B},
    {139, 0x052}, {143, 0xAFF}, {142, 0x2F8}, {141, 0x1A5},
    {55, 0x601}, {59, 0xF68}, {58, 0xF24}, {57, 0xA13},
    {195, 0x136}, {185, 0x9CF}, {199, 0x48F}, {197, 0x25A},
    {253, 0x606}, {241, 0xF9F}, {255, 0xF2C}, {254, 0xB18}
};
static void screen_palette_color(uint8_t index, uint16_t rgb) {
    vpoke((uint8_t)rgb, 0x1FA00UL + (uint16_t)index * 2);
    vpoke((uint8_t)(rgb >> 8), 0x1FA01UL + (uint16_t)index * 2);
}

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
                low = screen_style == SCREEN_STYLE_CYBERPUNK ? row_tile + (x & 3) : TILE_BG0;
                attribute = screen_style == SCREEN_STYLE_CYBERPUNK ? TILE_CYBERPUNK_FIRST >> 8 : 0;
            }
            VERA.data1 = low;
            VERA.data1 = attribute;
        }
    }
}

void screen_apply_style(void) {
    uint16_t i;
    uint8_t video = VERA.display.video;
    /* Reserved aliases: grid 232-235, floor 236-237, garbage ink 238, shadows 239. */
    static const uint8_t originals[] = {26, 28, 29, 30, 147, 148, 11, 11};
    VERA.display.video = 0;
    VERA.control = 0;
    VERA.address = 0xFA00;
    VERA.address_hi = 0x11;
    for(i = 0; i < sizeof(original_palette); i++)
        VERA.data0 = original_palette[i];
    for(i = 0; i < sizeof(originals); i++) {
        vpoke(original_palette[(uint16_t)originals[i] * 2], 0x1FA00UL + (232 + i) * 2);
        vpoke(original_palette[(uint16_t)originals[i] * 2 + 1], 0x1FA01UL + (232 + i) * 2);
    }
    /* Large-font white must stay independent of the dark card paper. */
    vpoke(original_palette[30 * 2], 0x1FA00UL + 240UL * 2);
    vpoke(original_palette[30 * 2 + 1], 0x1FA01UL + 240UL * 2);
    if(screen_style == SCREEN_STYLE_CYBERPUNK) {
        for(i = 0; i < sizeof(cyberpunk_palette) / sizeof(cyberpunk_palette[0]); i++)
            screen_palette_color(cyberpunk_palette[i].index, cyberpunk_palette[i].rgb);
    }
    screen_style_background();
    VERA.display.video = video;
}

void screen_apply_title_style(void) {
    uint8_t i;
    /* Same 16-color metropolis artwork, with a neon night palette. */
    static const uint16_t colors[16] = {
        0x012, 0x027, 0x306, 0x123, 0x245, 0x367, 0x489, 0x0BD,
        0x0EF, 0xA4D, 0xF2A, 0xBBF, 0x6CF, 0xFE2, 0x9EF, 0xEFF
    };
    if(screen_style == SCREEN_STYLE_CYBERPUNK)
        for(i = 0; i < 16; i++) screen_palette_color(i, colors[i]);
}
static uint8_t garbage_tiles_ready;
static uint8_t hud_tiles_ready;
static uint8_t banner_tiles_style = 255;
static uint8_t hud_pixels[128];
static char hud_last_digits[2][14][10];
static uint32_t hud_last_values[2][14];
static uint8_t hud_value_ready[2][14];
extern void __fastcall__ screen_hud_copy(const uint8_t* pixels, uint8_t count);

/**
 * @brief Initialize screen
 * 
 */
void screen_init_title() {
    uint16_t i;
    garbage_tiles_ready = 0;
    hud_tiles_ready = 0;
    banner_tiles_style = 255;

    VERA.display.video = 0;
    if(!palette_saved) {
        for(i = 0; i < sizeof(original_palette); i++)
            original_palette[i] = vpeek(0x1FA00UL + i);
        palette_saved = 1;
    }

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
 * @brief Initialize screen
 * 
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

    // enable both layers, no sprites
    VERA.display.video = (1 << 5) | (1 << 4) | 1;           // VGA and layers 1 and 2
}

/* Reuse the block silhouette and bevels with the existing grey palette.
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
 * @brief Initialize screen
 * 
 */
void screen_init_game() {
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

    // enable both layers, no sprites
    //VERA.display.video |= 0b00110000;
    VERA.display.video = (1 << 5) | (1 << 4) | 1;           // VGA and layers 1 and 2
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
void screen_create_background(uint8_t players) {
    // playfield
    screen_create_tiled_rectangle(1, 8, 10, 20);
    if(players == 2) {
        screen_create_tiled_rectangle(21, 8, 10, 20);
    }

}

/* The static HUD uses deduplicated tiles; numeric tiles belong to one field. */
void screen_create_scorebox(uint8_t players) {
    uint8_t row, col, limit = players == 1 ? 20 : 40;
    uint16_t tile, address;
    if(!hud_tiles_ready) {
        screen_load_tiles("hud.dat", 130UL * 256);
        memset(hud_last_digits, 0, sizeof(hud_last_digits));
        memset(hud_value_ready, 0, sizeof(hud_value_ready));
        hud_tiles_ready = 1;
    }
    for(row = 0; row < 30; row++) {
        for(col = 0; col < limit; col++) {
            tile = hud_map[row][col];
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

/* Store one native glyph; double pixels only when drawing the large score. */
static void screen_hud_digit_row(uint8_t* pixels, const uint8_t* glyph,
                                 uint8_t row, uint8_t scale) {
    uint8_t col, color;
    if(scale == 1) {
        memcpy(pixels, glyph + row * 5, 5);
    } else {
        glyph += (row >> 1) * 5;
        for(col = 0; col < 5; col++) {
            color = *glyph++;
            *pixels++ = color;
            *pixels++ = color;
        }
    }
}

/* Numeric fields update only changed glyphs; all other pixels stay cached. */
void screen_draw_hud_number(uint8_t player, uint8_t field, uint32_t value) {
    const struct HudField* f = &hud_fields[field];
    char text[11], ch;
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
        if(hud_last_digits[player][field][slot] == ch ||
           (!hud_last_digits[player][field][slot] && ch == ' ')) continue;
        if(ch != ' ') glyph = &hud_digits[ch - '0'][0][0];
        for(row = 0; row < f->height; row++) {
            if(ch == ' ') memset(hud_pixels, f->background, width);
            else screen_hud_digit_row(hud_pixels, glyph, row, scale);
            x = f->x + (uint16_t)player * 320 + slot * 6 * scale;
            y = f->y + row;
            remaining = width;
            written = 0;
            while(remaining) {
                tile = 129 + (uint16_t)hud_map[y >> 4][x >> 4];
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
        hud_last_digits[player][field][slot] = ch;
    }
    hud_last_values[player][field] = value;
    hud_value_ready[player][field] = 1;
}

/* Banners reuse a theme atlas; all lettering and icons are already baked. */
void screen_draw_player_banner(uint8_t player, uint8_t computer, uint8_t number) {
    uint8_t row, col, variant = (computer ? 2 : 0) + (number == 2 ? 1 : 0);
    uint16_t address;
    const uint16_t* tiles;
    if(banner_tiles_style != screen_style) {
        screen_load_tiles(screen_style == SCREEN_STYLE_CYBERPUNK ? "name-c.dat" : "name-l.dat",
                          (uint32_t)BANNER_TILE_FIRST * 256);
        screen_load_tiles(screen_style == SCREEN_STYLE_CYBERPUNK ? "name-cx.dat" : "name-lx.dat",
                          (uint32_t)BANNER_TILE_SPILL * 256);
        banner_tiles_style = screen_style;
    }
    for(row = 0; row < 2; row++) {
        address = (uint16_t)MAPBASE1 + ((uint16_t)row << 7) + ((1 + player * 20) << 1);
        tiles = banner_map[screen_style][variant][row];
        VERA.address = address;
        VERA.address_hi = 0x11;
        for(col = 0; col < 18; col++) {
            VERA.data0 = tiles[col];
            VERA.data0 = tiles[col] >> 8;
        }
    }
}

/**
 * @brief Draw "next piece" boxes
 * 
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
    screen_clear_rectangle(x, y, w, h);
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
 * @brief Position a single tile on the screen for layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param tile tile_id
 */
void screen_set_tile_layer1(uint8_t x, uint8_t y, uint8_t tile) {
    uint16_t address = (uint16_t)MAPBASE1 + ((uint16_t)y << 7) + ((uint16_t)x << 1);
    VERA.address = address;
    VERA.address_hi = 0x11; // Fixed VRAM bank, increment by one byte.
    VERA.data0 = tile;
    VERA.data0 = 0; // Clear extended HUD tile bits when drawing ordinary tiles.
}

/**
 * @brief Position a single tile on the screen for layer1
 * 
 * @param x x-position
 * @param y y-position
 * @param tile tile_id
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
 * @brief Redraw the playing field based on array
 * 
 * @param x 
 * @param playfield 
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

void screen_clear_row_cell(uint8_t xoffset, uint8_t row, uint8_t column) {
    screen_set_tile_layer1(xoffset + column, 27 - row, TILE_NONE);
}
