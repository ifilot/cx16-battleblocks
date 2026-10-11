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

#include "bagofseven.h"

/*
 * Seven-piece randomizer. Each shuffled bag contains every tetromino once;
 * the game keeps a second bag ready for previews and rollover.
 */

/**
 * @brief Shuffle all seven tetromino types and reset the bag cursor
 *
 * @param bos Bag to fill and reset
 */
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
