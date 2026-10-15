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

#include "menu.h"
#include "ai.h"
#include "input.h"
#include <string.h>
#include "game.h"
#include "sound.h"
#include "version_generated.h"

/* Menu functions are synchronous and nonrecursive. Static locals keep this
 * UI within resident RAM without reducing the reserved software stack. */
#pragma static-locals(on)

/*
 * Tile-font menus, theme selection, session options and application details.
 * The selected game mode is returned to the application scene loop.
 */

#define MENU_STYLE 4
#define MENU_OPTIONS 5
#define MENU_ABOUT 6
#define MENU_LAST MENU_EXIT
#define MENU_TEXT_X 18
#define MENU_CURSOR_X 16
#define MENU_CPU_X 1
#define MENU_CPU_Y 4
static const char* const style_names[] = {"COMMANDER X16", "CYBERPUNK", "FOUNDRY", "SYNTHWAVE", "MATRIX"};

/**
 * @brief Draw the main menu, all seven pieces and the selection cursor
 */
static void menu_draw(uint8_t selected);

/**
 * @brief Clear menu layers and apply the selected theme
 */
static void menu_clear(void) {
    screen_transition_begin();
    screen_fill_layer(LAYER0, TILE_BG0);
    screen_apply_style();
    screen_fill_layer(LAYER1, TILE_NONE);
}

/**
 * @brief Draw text in the menu font, clipped to the 40-by-30 viewport
 *
 * @param x   Horizontal tile coordinate
 * @param y   Vertical tile coordinate
 * @param str Null-terminated text; letters are converted to uppercase
 */
void menu_settext(uint8_t x, uint8_t y, const char* str) {
    uint8_t ch;
    uint32_t map_base_addr = MAPBASE1 + (y << 7) + (x << 1);
    if(y >= 30 || x >= 40) {
        return;
    }
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (1 << 5);

    while(*str != 0 && x++ < 40) {
        ch = *str++;
        if(ch >= 'a' && ch <= 'z') {
            ch -= 'a' - 'A';
        }
        if(ch >= '0' && ch <= '9') {
            VERA.data0 = ch - '0' + 24;
        }
        else if(ch >= 'A' && ch <= 'Z') {
            VERA.data0 = ch - 'A' + 96;
        } else if(ch == '.') {
            VERA.data0 = 36;
        } else if(ch == '/') {
            VERA.data0 = 37;
        } else if(ch == '-') {
            VERA.data0 = 38;
        } else if(ch == ':') {
            VERA.data0 = 39;
        } else if(ch == '%') {
            VERA.data0 = 40;
        } else if(ch == '>') {
            VERA.data0 = TILE_SELECT_CURSOR;
        } else {
            VERA.data0 = TILE_NONE;
        }
    }
}

/**
 * @brief Center menu text horizontally on the requested row
 *
 * @param y   Vertical tile coordinate
 * @param str Null-terminated text; letters are converted to uppercase
 */
void menu_centertext(uint8_t y, const char* str) {
    uint8_t length = strlen(str);
    menu_settext(length < 40 ? (40 - length) / 2 : 0, y, str);
}

/**
 * @brief Show the selected option's description and relevant controls
 */
static void menu_description(uint8_t mode) {
    static const char* const descriptions[] = {
        "CLEAR LINES  BEAT YOUR SCORE", "BATTLE THE COMPUTER",
        "LOCAL TWO PLAYER BATTLE", "COMPUTER VERSUS COMPUTER",
        "PREVIEW FIVE VISUAL STYLES", "MUSIC AND SOUND EFFECTS",
        "VERSION  CREDITS  SYSTEM DETAILS", "RETURN TO THE BASIC PROMPT"
    };
    menu_settext(2, 25, "                                    ");
    menu_settext(2, 26, "                                    ");
    menu_centertext(25, descriptions[mode]);
    if(mode < GAME_MODE_DEMO) menu_centertext(26, "P1 ARROWS SPACE  P2 WASD Q");
    else if(mode == MENU_STYLE) menu_centertext(26, "ENTER TO CHANGE STYLE");
}

/**
 * @brief Draw a tetromino in its starting orientation
 */
static void menu_piece(uint8_t x, uint8_t y, uint8_t type) {
    uint8_t i;
    for(i = 0; i < 4; i++) {
        /* Only the I piece starts at column zero; align its left edge. */
        screen_set_tile_layer1(x + pieces_start_table[type][i][0] - (type != 0),
                              y + pieces_start_table[type][i][1] + 1,
                              TYPE_TO_TILEID(type));
    }
}

/**
 * @brief Show the computer difficulty, with arrows toward the other levels
 */
static void menu_difficulty(void) {
    static const char levels[][7] = {"NOVICE", " EASY ", "NORMAL", " HARD ", "EXPERT"};
    uint16_t address = (uint16_t)MAPBASE1 + ((MENU_CPU_Y + 1) << 7) + ((MENU_CPU_X + 3) << 1);
    menu_settext(MENU_CPU_X + 4, MENU_CPU_Y + 1, levels[ai_difficulty]);
    VERA.address = address;
    VERA.address_hi = 0x11;
    VERA.data0 = ai_difficulty ? TILE_SELECT_CURSOR : TILE_NONE;
    VERA.data0 = 0x04; // A horizontally flipped cursor points left.
    screen_set_tile_layer1(MENU_CPU_X + 10, MENU_CPU_Y + 1,
                           ai_difficulty < AI_EXPERT ? TILE_SELECT_CURSOR : TILE_NONE);
}

/**
 * @brief Change the computer difficulty from the main menu
 */
static void menu_set_difficulty(uint8_t level) {
    ai_difficulty = level;
    sound_menu_effect(SFX_MENU_SELECT);
    menu_difficulty();
}

/**
 * @brief Switch theme and refresh its palette, background and label
 */
static void menu_toggle_style(void) {
    sound_menu_effect(SFX_MENU_SELECT);
    if(++screen_style >= SCREEN_STYLE_COUNT) screen_style = SCREEN_STYLE_COMMANDER_X16;
    screen_transition_begin();
    screen_apply_style();
    screen_draw_menu_title();
    screen_draw_computer_face(MENU_CPU_X, MENU_CPU_Y);
    screen_transition_end(0x31);
}

/**
 * @brief Preview each theme and apply it, or restore the original on Escape
 */
static void menu_styles(void) {
    uint8_t original = screen_style;
    uint8_t selected = original;
    uint8_t previous, key, i;
    static const char* const descriptions[] = {
        "X16 WHITE AND BLUE", "NEON CIRCUIT BOARDS", "RIVETED COPPER", "NEON SUNSET", "PHOSPHOR TERMINAL"
    };
    menu_clear();
    menu_centertext(2, "CHOOSE YOUR STYLE");
    for(i = 0; i < SCREEN_STYLE_COUNT; i++) {
        /* Leave separate tile rows for this bottom rim and the next top rim. */
        screen_create_lined_rectangle(2, 5 + i * 4, 22, 2);
        menu_settext(7, 5 + i * 4, style_names[i]);
        screen_set_tile_layer1(5, 5 + i * 4, 25 + i);
        menu_settext(5, 6 + i * 4, descriptions[i]);
    }
    screen_set_tile_layer1(22, 5 + original * 4, TILE_STYLE_CHECK);
    screen_create_lined_rectangle(27, 6, 10, 18);
    menu_settext(29, 7, "PREVIEW");
    for(i = 0; i < 7; i++) menu_piece(28 + (i % 2) * 5, 8 + (i / 2) * 4, i);
    screen_set_tile_layer1(3, 5 + selected * 4, TILE_SELECT_CURSOR);
    menu_centertext(26, "UP DOWN PREVIEW  ENTER APPLY");
    menu_centertext(28, "1 2 3 4 5 CHOOSE  ESC CANCEL");
    screen_transition_end(0x31);
    while(1) {
        key = input_key();
        previous = selected;
        if(key == KEYCODE_ESCAPE) {
            screen_style = original;
            sound_menu_effect(SFX_MENU_BACK);
            return;
        }
        if(key == KEYCODE_RETURN || key == KEYCODE_SPACE) {
            sound_menu_effect(SFX_MENU_SELECT);
            return;
        }
        if(key == KEYCODE_UP || key == KEYCODE_LEFT)
            selected = selected == 0 ? SCREEN_STYLE_COUNT - 1 : selected - 1;
        else if(key == KEYCODE_DOWN || key == KEYCODE_RIGHT || key == 'T' || key == 't')
            selected = selected == SCREEN_STYLE_COUNT - 1 ? 0 : selected + 1;
        else if(key >= '1' && key <= '5') selected = key - '1';
        if(previous != selected) {
            sound_menu_effect(SFX_MENU_MOVE);
            screen_transition_begin();
            screen_style = selected;
            screen_apply_style();
            screen_set_tile_layer1(3, 5 + previous * 4, TILE_NONE);
            screen_set_tile_layer1(3, 5 + selected * 4, TILE_SELECT_CURSOR);
            screen_transition_end(0x31);
        }
    }
}

/**
 * @brief Show application details until a return key is pressed
 */
static void menu_about(void) {
    uint8_t key, i;
    static const char* const details[] = {
        "VERSION V" GAME_VERSION, "CREATED BY IVO FILOT",
        "COMMANDER X16  65C02 AT 8 MHZ", "640X480 VERA TILE GRAPHICS",
        "16 PSG VOICES  ZSMKIT AUDIO", "C AND 65C02 ASSEMBLY  CC65",
        "GITHUB REPOSITORY", "GITHUB.COM/IFILOT/CX16-BATTLEBLOCKS"
    };
    static const uint8_t rows[] = {6, 9, 12, 14, 16, 18, 21, 23};
    menu_clear();
    menu_centertext(2, "ABOUT BATTLE BLOCKS");
    screen_create_lined_rectangle(1, 4, 38, 22);
    for(i = 0; i < 8; ++i) menu_centertext(rows[i], details[i]);
    menu_centertext(28, "ENTER SPACE OR ESC TO RETURN");
    screen_transition_end(0x31);
    do {
        key = input_key();
    } while(key != KEYCODE_ESCAPE && key != KEYCODE_RETURN && key != KEYCODE_SPACE);
    sound_menu_effect(SFX_MENU_BACK);
}

/**
 * @brief Refresh audio flags and volume percentages
 */
static void menu_options_values(void) {
    static const char volumes[11][5] = {
        "0%  ", "10% ", "20% ", "30% ", "40% ", "50% ",
        "60% ", "70% ", "80% ", "90% ", "100%"
    };
    menu_settext(23, 8, sound_music_enabled ? "ON " : "OFF");
    menu_settext(28, 8, volumes[sound_music_volume]);
    menu_settext(23, 12, sound_effects_enabled ? "ON " : "OFF");
    menu_settext(28, 12, volumes[sound_effects_volume]);
}

/**
 * @brief Run the options menu and retain settings for this session
 */
static void menu_options(void) {
    uint8_t selected = 0;
    uint8_t previous, key;
    uint8_t* volume;
    menu_clear();
    menu_centertext(2, "OPTIONS");
    screen_create_lined_rectangle(3, 5, 34, 14);
    menu_settext(7, 8, "MUSIC");
    menu_settext(7, 12, "SOUNDS");
    menu_settext(7, 16, "BACK");
    menu_options_values();
    screen_set_tile_layer1(5, 8, TILE_SELECT_CURSOR);
    menu_centertext(25, "UP DOWN SELECT  LEFT RIGHT CHANGE");
    menu_centertext(27, "ENTER SPACE TOGGLE  ESC BACK");
    screen_transition_end(0x31);
    while(1) {
        key = input_key();
        previous = selected;
        if(key == KEYCODE_ESCAPE ||
           (selected == 2 && (key == KEYCODE_RETURN || key == KEYCODE_SPACE))) {
            sound_menu_effect(SFX_MENU_BACK);
            return;
        }
        if(key == KEYCODE_UP) {
            selected = selected == 0 ? 2 : selected - 1;
        } else if(key == KEYCODE_DOWN) {
            selected = selected == 2 ? 0 : selected + 1;
        } else if(selected < 2) {
            volume = selected == 0 ? &sound_music_volume : &sound_effects_volume;
            if(key == KEYCODE_RETURN || key == KEYCODE_SPACE) {
                if(selected == 0) sound_music_enabled ^= 1;
                else sound_effects_enabled ^= 1;
            } else if(key == KEYCODE_LEFT) {
                if(*volume) --*volume;
            } else if(key == KEYCODE_RIGHT) {
                if(*volume < 10) ++*volume;
            } else continue;
            sound_apply_options();
            sound_menu_effect(SFX_MENU_SELECT);
            menu_options_values();
        }
        if(previous != selected) {
            sound_menu_effect(SFX_MENU_MOVE);
            screen_set_tile_layer1(5, 8 + previous * 4, TILE_NONE);
            screen_set_tile_layer1(5, 8 + selected * 4, TILE_SELECT_CURSOR);
        }
    }
}

/**
 * @brief Draw the main menu, all seven pieces and the selection cursor
 */
static void menu_draw(uint8_t selected) {
    uint8_t i;
    static const char* const labels[] = {
        "1 SINGLE PLAYER", "2 VERSUS COMPUTER", "3 TWO PLAYERS",
        "4 COMPUTER DEMO", "5 SELECT STYLE", "6 OPTIONS", "7 ABOUT", "8 EXIT"
    };
    menu_clear();
    screen_draw_menu_title();
    screen_draw_computer_face(MENU_CPU_X, MENU_CPU_Y);
    menu_settext(MENU_CPU_X + 3, MENU_CPU_Y, "COMPUTER");
    menu_difficulty();
    menu_centertext(5, "MAIN MENU");
    screen_create_lined_rectangle(2, 7, 10, 17);
    screen_create_lined_rectangle(15, 7, 23, 17);
    for(i = 0; i <= MENU_LAST; ++i) menu_settext(MENU_TEXT_X, 8 + i * 2, labels[i]);
    /* Two columns, with two empty rows between successive pairs. */
    for(i = 0; i < 7; ++i) menu_piece(3 + (i % 2) * 5, 8 + (i / 2) * 4, i);
    menu_centertext(28, "UP DOWN OR 1 TO 8 SELECT  ENTER OPEN");
    menu_centertext(29, "LEFT RIGHT COMPUTER  T QUICK STYLE");
    screen_set_tile_layer1(MENU_CURSOR_X, 8 + selected * 2, TILE_SELECT_CURSOR);
    menu_description(selected);
    screen_transition_end(0x31);
}

/**
 * @brief Run the main menu and return the selected game mode
 *
 * @param initial_mode Zero-based initial menu selection, 0..6
 *
 * @return Selected GAME_MODE_* value
 */
uint8_t menu_select(uint8_t initial_mode) {
    uint8_t selected = initial_mode;
    uint8_t previous;
    uint8_t key;
    menu_draw(selected);

    while(1) {
        key = input_key();
        previous = selected;
        switch(key) {
            case KEYCODE_UP:
            case 'W':
            case 'w':
                selected = selected == 0 ? MENU_LAST : selected - 1;
                break;
            case KEYCODE_DOWN:
            case 'S':
            case 's':
                selected = selected == MENU_LAST ? 0 : selected + 1;
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
                selected = key - '1';
                break;
            case KEYCODE_LEFT:
            case 'A':
            case 'a':
                if(ai_difficulty) menu_set_difficulty(ai_difficulty - 1);
                break;
            case KEYCODE_RIGHT:
            case 'D':
            case 'd':
                if(ai_difficulty < AI_EXPERT) menu_set_difficulty(ai_difficulty + 1);
                break;
            case KEYCODE_RETURN:
            case KEYCODE_SPACE:
                if(selected == MENU_EXIT) {
                    sound_menu_effect(SFX_MENU_SELECT);
                    if(menu_confirm("EXIT TO BASIC")) return MENU_EXIT;
                }
                else if(selected == MENU_STYLE || selected == MENU_OPTIONS || selected == MENU_ABOUT) {
                    sound_menu_effect(SFX_MENU_SELECT);
                    if(selected == MENU_STYLE) menu_styles();
                    else if(selected == MENU_OPTIONS) menu_options();
                    else menu_about();
                    menu_draw(selected);
                }
                else {
                    sound_menu_effect(SFX_MENU_SELECT);
                    return selected;
                }
                break;
            case 'T':
            case 't':
                menu_toggle_style();
                break;
            case KEYCODE_ESCAPE:
                sound_menu_effect(SFX_MENU_BACK);
                break;
        }
        if(previous != selected) {
            sound_menu_effect(SFX_MENU_MOVE);
            screen_set_tile_layer1(MENU_CURSOR_X, 8 + previous * 2, TILE_NONE);
            screen_set_tile_layer1(MENU_CURSOR_X, 8 + selected * 2, TILE_SELECT_CURSOR);
            menu_description(selected);
        }
    }
}
