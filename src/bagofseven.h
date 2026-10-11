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

#ifndef _BAG_OF_SEVEN_H
#define _BAG_OF_SEVEN_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/*
 * Seven-piece randomizer. Each shuffled bag contains every tetromino once;
 * the game keeps a second bag ready for previews and rollover.
 */

struct BagOfSeven {
    uint8_t pieces[7];                           // Permutation of the seven tetromino types
    uint8_t idx;                                 // Next piece index; rollover occurs after index 6
};

struct PieceList {
    struct BagOfSeven bag1;                      // Current bag used for spawning and preview
    struct BagOfSeven bag2;                      // Prepared bag used on the next rollover
};

/**
 * @brief Shuffle all seven tetromino types and reset the bag cursor
 *
 * @param bos Bag to fill and reset
 */
void bos_generate(struct BagOfSeven* bos);

#endif // _BAG_OF_SEVEN_H
