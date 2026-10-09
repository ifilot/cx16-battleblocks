#include "menu.h"
#include <string.h>
#include "game.h"
#include "sound.h"

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
    menu_settext(2, 26, "                                    ");
    menu_settext(2, 24, "                                    ");
    menu_settext(2, 25, "                                    ");
    if(mode < GAME_MODE_DEMO) menu_centertext(25, "P1 ARROWS  SPACE DROP");
    switch(mode) {
        case 4:
            menu_centertext(24, "CLASSIC OR CYBERPUNK COLORS");
            menu_centertext(25, "ENTER TO CHANGE STYLE");
            break;
        case GAME_MODE_SINGLE:
            menu_centertext(24, "CLEAR LINES AND BEAT YOUR SCORE");
            break;
        case GAME_MODE_COMPUTER:
            menu_centertext(24, "RACE AGAINST THE COMPUTER");
            break;
        case GAME_MODE_DEMO:
            menu_centertext(24, "COMPUTER VERSUS COMPUTER");
            menu_centertext(25, "SIT BACK AND ENJOY");
            break;
        case GAME_MODE_LOCAL:
            menu_centertext(24, "TWO PLAYERS ON ONE KEYBOARD");
            menu_centertext(26, "P2 WASD  Q DROP");
            break;
    }
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
    menu_settext(11, 21, "5 STYLE             ");
    menu_settext(19, 21, screen_style == SCREEN_STYLE_CYBERPUNK ? "CYBERPUNK" : "CLASSIC");
}

static void menu_toggle_style(void) {
    sound_menu_effect(SFX_MENU_SELECT);
    screen_style ^= 1;
    screen_apply_style();
    menu_style_label();
}

uint8_t menu_select(uint8_t initial_mode) {
    uint8_t selected = initial_mode;
    uint8_t previous;
    uint8_t key;

    menu_centertext(2, "BATTLE BLOCKS");
    menu_centertext(5, "SELECT GAME");
    screen_create_lined_rectangle(7, 7, 26, 16);
    menu_settext(11, 9, "1 SINGLE PLAYER");
    menu_settext(11, 12, "2 VERSUS COMPUTER");
    menu_settext(11, 15, "3 TWO PLAYERS");
    menu_settext(11, 18, "4 COMPUTER DEMO");
    menu_style_label();
    menu_piece(29, 17, 4);
    menu_piece(29, 8, 1);
    menu_piece(29, 11, 2);
    menu_piece(29, 14, 3);
    menu_centertext(28, "UP DOWN SELECT  ENTER START");
    menu_centertext(29, "T CHANGE STYLE  ESC BACK");
    screen_set_tile_layer1(9, 9 + selected * 3, TILE_SELECT_CURSOR);
    menu_description(selected);

    while(1) {
        key = cbm_k_getin();
        previous = selected;
        switch(key) {
            case KEYCODE_UP:
            case 'W':
            case 'w':
                selected = selected == 0 ? 4 : selected - 1;
                break;
            case KEYCODE_DOWN:
            case 'S':
            case 's':
                selected = selected == 4 ? 0 : selected + 1;
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
                selected = key - '1';
                break;
            case KEYCODE_RETURN:
            case KEYCODE_SPACE:
                if(selected == 4) menu_toggle_style();
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
            screen_set_tile_layer1(9, 9 + previous * 3, TILE_NONE);
            screen_set_tile_layer1(9, 9 + selected * 3, TILE_SELECT_CURSOR);
            menu_description(selected);
        }
    }
}
