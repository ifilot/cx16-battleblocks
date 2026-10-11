#include <cx16.h>
#include <cbm.h>
#include <string.h>
#define RESOURCE_TABLE
#include "resources_generated.h"

/* MACPTR streams directly to VERA; no graphics staging buffer or RAM banks. */
extern unsigned char __fastcall__ resources_read_vram(unsigned char count);

void screen_load_tiles(unsigned char resource, unsigned long addr) {
    /* Scene loads are synchronous; static counters keep the 6502 code small. */
    static unsigned char i, count;
    static unsigned int remaining;
    static unsigned char seek[6] = {'p', 2, 0, 0, 0, 0};
    cbm_k_setnam("graphics.dat,s,r");
    cbm_k_setlfs(2, 8, 2);
    if(cbm_k_open()) return;
    cbm_k_setnam("");
    cbm_k_setlfs(15, 8, 15);
    if(cbm_k_open()) {
        cbm_k_close(2);
        return;
    }
    memcpy(seek + 3, graphics[resource].page, 2);
    /* CMDR-DOS P command: channel followed by a 32-bit little-endian offset. */
    cbm_k_ckout(15);
    for(i = 0; i < sizeof(seek); ++i) cbm_k_bsout(seek[i]);
    cbm_k_clrch();
    if(!cbm_k_chkin(2)) {
        VERA.control = 0;
        VERA.address = addr;
        VERA.address_hi = 0x10 | (addr >> 16);
        remaining = graphics[resource].size;
        while(remaining) {
            count = (remaining >> 8) ? 255 : remaining;
            if(resources_read_vram(count) != count) break;
            remaining -= count;
        }
    }
    cbm_k_clrch();
    cbm_k_close(2);
    cbm_k_close(15);
}
