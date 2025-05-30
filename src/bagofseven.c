#include "bagofseven.h"

void bos_generate(struct BagOfSeven* bos) {
    uint8_t i,j,tmp;

    for (i = 0; i < 7; ++i) {
        bos->pieces[i] = i;
    }

    // Fisher-Yates shuffle using rand()
    for (i = 6; i > 0; --i) {
        j = rand() % (i + 1);
        tmp = bos->pieces[i];
        bos->pieces[i] = bos->pieces[j];
        bos->pieces[j] = tmp;
    }

    bos->idx = 0;
}