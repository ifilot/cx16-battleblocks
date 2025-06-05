#include "menu.h"

void menu_clear() {
    uint8_t i,j;
    uint32_t map_base_addr = MAPBASE0;
    VERA.address = map_base_addr;
    VERA.address_hi = map_base_addr >> 16;
    VERA.address_hi |= (1 << 4); // 1 byte increments

    for(i=0; i<32; i++) {
        for(j=0; j<64; j++) {
            VERA.data0 = 0x00;
        }
    }
}