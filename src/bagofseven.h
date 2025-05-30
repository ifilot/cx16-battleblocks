#ifndef _BAG_OF_SEVEN_H
#define _BAG_OF_SEVEN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

struct BagOfSeven {
    uint8_t pieces[7];
    uint8_t idx;
};

struct PieceList {
    struct BagOfSeven bag1;
    struct BagOfSeven bag2;
};

void bos_generate(struct BagOfSeven* bos);

#endif