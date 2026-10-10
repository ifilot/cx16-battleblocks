#include "menu.h"
#include <string.h>
#include "game.h"
#include "sound.h"
#include "version_generated.h"

#define MENU_STYLE 4
#define MENU_OPTIONS 5
#define MENU_ABOUT 6
#define MENU_LAST MENU_ABOUT

static void menu_draw(uint8_t selected);

static void menu_clear(void) {
    screen_transition_begin();
    screen_fill_layer(LAYER0, TILE_BG0);
    screen_apply_style();
    screen_fill_layer(LAYER1, TILE_NONE);
}

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
        } else {
            VERA.data0 = TILE_NONE;
        }
    }
}

void menu_centertext(uint8_t y, const char* str) {
    uint8_t length = strlen(str);
    menu_settext(length < 40 ? (40 - length) / 2 : 0, y, str);
}

static void menu_description(uint8_t mode) {
    static const char* const descriptions[] = {
        "CLEAR LINES  BEAT YOUR SCORE", "BATTLE THE COMPUTER",
        "LOCAL TWO PLAYER BATTLE", "COMPUTER VERSUS COMPUTER",
        "CLASSIC OR CYBERPUNK", "MUSIC AND SOUND EFFECTS",
        "VERSION  CREDITS  SYSTEM DETAILS"
    };
    menu_settext(2, 24, "                                    ");
    menu_settext(2, 25, "                                    ");
    menu_centertext(24, descriptions[mode]);
    if(mode < GAME_MODE_DEMO) menu_centertext(25, "P1 ARROWS SPACE  P2 WASD Q");
    else if(mode == MENU_STYLE) menu_centertext(25, "ENTER TO CHANGE STYLE");
}

static void menu_piece(uint8_t x, uint8_t y, uint8_t type) {
    uint8_t i;
    for(i = 0; i < 4; i++) {
        screen_set_tile_layer1(x + pieces_start_table[type][i][0] - 1,
                              y + pieces_start_table[type][i][1] + 1,
                              TYPE_TO_TILEID(type));
    }
}

static void menu_style_label(void) {
    menu_settext(19, 16, screen_style == SCREEN_STYLE_CYBERPUNK ? "CYBERPUNK" : "CLASSIC  ");
}

static void menu_toggle_style(void) {
    sound_menu_effect(SFX_MENU_SELECT);
    screen_style ^= 1;
    screen_transition_begin();
    menu_style_label();
    screen_apply_style();
    screen_transition_end(0x31);
}

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
        key = cbm_k_getin();
    } while(key != KEYCODE_ESCAPE && key != KEYCODE_RETURN && key != KEYCODE_SPACE);
    sound_menu_effect(SFX_MENU_BACK);
}

static void menu_options_values(void) {
    static const char volumes[11][5] = {
        "0%  ", "10% ", "20% ", "30% ", "40% ", "50% ",
        "60% ", "70% ", "80% ", "90% ", "100%"
    };
    menu_settext(23, 10, sound_music_enabled ? "ON " : "OFF");
    menu_settext(28, 10, volumes[sound_music_volume]);
    menu_settext(23, 14, sound_effects_enabled ? "ON " : "OFF");
    menu_settext(28, 14, volumes[sound_effects_volume]);
}

static void menu_options(void) {
    uint8_t selected = 0;
    uint8_t previous, key;
    uint8_t* volume;
    menu_clear();
    menu_centertext(2, "AUDIO OPTIONS");
    screen_create_lined_rectangle(3, 5, 34, 18);
    menu_settext(7, 10, "MUSIC");
    menu_settext(7, 14, "SOUNDS");
    menu_settext(7, 18, "BACK");
    menu_options_values();
    screen_set_tile_layer1(5, 10, TILE_SELECT_CURSOR);
    menu_centertext(25, "UP DOWN SELECT  LEFT RIGHT VOLUME");
    menu_centertext(27, "ENTER SPACE TOGGLE  ESC BACK");
    screen_transition_end(0x31);
    while(1) {
        key = cbm_k_getin();
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
            screen_set_tile_layer1(5, 10 + previous * 4, TILE_NONE);
            screen_set_tile_layer1(5, 10 + selected * 4, TILE_SELECT_CURSOR);
        }
    }
}

static void menu_draw(uint8_t selected) {
    uint8_t i;
    static const char* const labels[] = {
        "1 SINGLE PLAYER", "2 VERSUS COMPUTER", "3 TWO PLAYERS",
        "4 COMPUTER DEMO", "5 STYLE", "6 OPTIONS", "7 ABOUT"
    };
    menu_clear();
    menu_centertext(2, "BATTLE BLOCKS");
    menu_centertext(5, "MAIN MENU");
    screen_create_lined_rectangle(7, 7, 26, 16);
    for(i = 0; i < 7; ++i) menu_settext(11, 8 + i * 2, labels[i]);
    menu_style_label();
    for(i = 0; i < 4; ++i) menu_piece(29, 7 + i * 2, i + 1);
    menu_centertext(28, "UP DOWN SELECT  ENTER OPEN");
    menu_centertext(29, "T CHANGE STYLE  ESC BACK");
    screen_set_tile_layer1(9, 8 + selected * 2, TILE_SELECT_CURSOR);
    menu_description(selected);
    screen_transition_end(0x31);
}

uint8_t menu_select(uint8_t initial_mode) {
    uint8_t selected = initial_mode;
    uint8_t previous;
    uint8_t key;
    menu_draw(selected);

    while(1) {
        key = cbm_k_getin();
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
                selected = key - '1';
                break;
            case KEYCODE_RETURN:
            case KEYCODE_SPACE:
                if(selected == MENU_STYLE) menu_toggle_style();
                else if(selected == MENU_OPTIONS || selected == MENU_ABOUT) {
                    sound_menu_effect(SFX_MENU_SELECT);
                    if(selected == MENU_OPTIONS) menu_options();
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
                return MENU_BACK;
        }
        if(previous != selected) {
            sound_menu_effect(SFX_MENU_MOVE);
            screen_set_tile_layer1(9, 8 + previous * 2, TILE_NONE);
            screen_set_tile_layer1(9, 8 + selected * 2, TILE_SELECT_CURSOR);
            menu_description(selected);
        }
    }
}
