#include <cx16.h>
#include <cbm.h>
#define RESOURCE_TABLE
#include "resources_generated.h"

/* MACPTR streams directly to VERA; no graphics staging buffer or RAM banks. */
extern unsigned int __fastcall__ resources_read_vram(unsigned char count);

void screen_load_tiles(unsigned char resource, unsigned long addr) {
    unsigned char i, count;
    unsigned int remaining, read;
    unsigned long offset;
    unsigned char seek[6] = {'p', 2, 0, 0, 0, 0};
    cbm_k_setnam("graphics.dat,s,r");
    cbm_k_setlfs(2, 8, 2);
    if(cbm_k_open()) return;
    cbm_k_setnam("");
    cbm_k_setlfs(15, 8, 15);
    if(cbm_k_open()) {
        cbm_k_close(2);
        return;
    }
    offset = graphics[resource].offset;
    seek[2] = offset;
    seek[3] = offset >> 8;
    seek[4] = offset >> 16;
    seek[5] = offset >> 24;
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
            count = remaining > 255 ? 255 : remaining;
            read = resources_read_vram(count);
            if(read != count) break;
            remaining -= read;
        }
    }
    cbm_k_clrch();
    cbm_k_close(2);
    cbm_k_close(15);
}
