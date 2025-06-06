#include "menu.h"

void menu_settext(uint8_t x, uint8_t y, const char* str) {
    uint32_t map_base_addr = MAPBASE1 + (y << 7) + (x << 1);
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (1 << 5);
    
    while(*str != 0) {
        if(*str >= '0' && *str <= '9') {
            VERA.data0 = *str - '0' + 24;
        }
        else if(*str != ' ') {
            VERA.data0 = *str - 'a' + 96;
        } else {
            VERA.data0 = TILE_NONE;
        }
        str++;
    }
}