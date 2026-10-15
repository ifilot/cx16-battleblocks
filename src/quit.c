#include "menu.h"
#include "input.h"
#include "screen.h"

/* Banks 3 and 4 are scratch space between effects (2) and music (10 onward). */
extern void __fastcall__ screen_modal_copy(uint8_t restore);
uint8_t game_quit_open;

uint8_t menu_quit(void) {
    return menu_confirm("QUIT MATCH");
}

uint8_t __fastcall__ menu_confirm(const char* title) {
    uint8_t key, quit = 0;
    input_freeze_begin();
    game_quit_open = 1;
    screen_modal_copy(0);
    screen_create_lined_rectangle(7, 11, 26, 9);
    menu_centertext(13, title);
    menu_centertext(18, "ARROWS ENTER  ESC CANCEL");
    while(1) {
        /* Reserve both cursor slots so neither label moves on selection. */
        menu_centertext(16, quit ? "> YES     NO" : "  YES   > NO");
        input_freeze_tick();
        key = input_key();
        if(key == KEYCODE_ESCAPE || key == 'n' || key == 'N') { quit = 0; break; }
        if(key == 'y' || key == 'Y') { quit = 1; break; }
        if(key == KEYCODE_RETURN || key == KEYCODE_SPACE) break;
        if(key == KEYCODE_LEFT || key == KEYCODE_UP) quit = 1;
        if(key == KEYCODE_RIGHT || key == KEYCODE_DOWN) quit = 0;
    }
    screen_modal_copy(1);
    input_freeze_tick();
    game_quit_open = 0;
    return quit;
}
