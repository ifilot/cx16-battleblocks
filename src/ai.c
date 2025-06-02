#include "ai.h"

struct PlayFieldEvaluation pfeval;

void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t i,j;
    uint8_t piece[4][2];
    uint8_t miny = 20;
    uint8_t minx = 20;
    uint8_t rot_id = 0;
    uint8_t type = gamestate->piece_type;

    memcpy(piece, gamestate->piece_position, 8);

    // find minimum height of current piece
    for(j=0; j<4; j++) { // loop over rotations
        miny = 20;
        minx = 20;
        for(i=0; i<4; i++) {    // loop over blocks
            if(piece[i][0] < minx) {
                minx = piece[i][0];
            }
            if(piece[i][1] < miny) {
                miny = piece[i][1];
            }
        }

        // store game piece
        for(i=0; i<4; i++) {
            evaluation->pieces[j][i][0] = piece[i][0] - minx;
            evaluation->pieces[j][i][1] = piece[i][1] - miny;
        }

        // perform rotation for next item
        for(i=0; i<4; i++) {
            piece[i][0] += piece_rotation_table[type][rot_id][i][0];
            piece[i][1] -= piece_rotation_table[type][rot_id][i][1];
        }
        rot_id++;
    }

    // reset evaluation
    evaluation->rot_id = 0;
    evaluation->offset_id = 0;
    evaluation->best_rot_id = 0xFF;
    evaluation->best_offset_id = 0xFF;
}

uint8_t ai_calculate_next_score(struct GameState* gamestate, 
                                struct PlayFieldEvaluation* evaluation) {
    uint8_t i;
    int8_t row;
    uint8_t x,y;
    uint8_t rot_id = evaluation->rot_id;
    uint8_t offset_id = evaluation->offset_id;
    uint8_t hit = 0;
    uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH];
    uint8_t noeval = 0;

    if(rot_id >= 4) {
        if(evaluation->best_rot_id == 0xFF) {
            ai_find_best_score(evaluation);
        }
        return 1;
    }

    // create a copy of current play field
    memcpy(playfield, gamestate->playfield, PLAYFIELD_WIDTH * PLAYFIELD_HEIGHT);

    // find lowest position piece can be placed; we start at 18 rather
    // than 19 to ensure that a vertical I-piece can be assessed
    for(row=19; row>=0; row--) {
        hit = 1;
        for(i=0; i<4; i++) {
            x = evaluation->pieces[rot_id][i][0] + offset_id;
            y = row + evaluation->pieces[rot_id][i][1];

            if(gamestate->playfield[y][x] != 0xFF) {
                hit = 0;
                break;
            }
        }

        if(hit == 0) {
            break;
        }
    }

     
    row++;
    noeval = 0;
    for(i=0; i<4; i++) {
        x = evaluation->pieces[rot_id][i][0] + offset_id;
        y = evaluation->pieces[rot_id][i][1] + row;
        if(x > 9) {
            noeval = 1;
            break;
        }
        playfield[y][x] = 0x00; // tile_id does not matter, as long it is not 0xFF
    }

    if(noeval == 1) {
        evaluation->score[rot_id][offset_id] = 0xFFFFFFFF;
    } else {
        evaluation->score[rot_id][offset_id] = ai_calculate_playfield_score(playfield);
    }

    if((evaluation->offset_id++) == 9) {
        evaluation->rot_id++;
        evaluation->offset_id = 0;
    }

    return 0;
}

/**
 * @brief Evaluate the weight function given a playfield
 * 
 * Initial parameters have been obtained from: https://codemyroad.wordpress.com/2013/04/14/tetris-ai-the-near-perfect-player/,
 * but have been tweaked for optimal performance in this game. Specifically, in constrast to the original implementation,
 * creation of holes has been significantly penalized.
 * 
 * Note that full floating point arithmetic would be too costly for this system, so integer-based evaluation is
 * used. Still, integer-based multiplication is relatively expensive, so we opted for using bitshifts + regular
 * addition. We use approximately 2-3 digits of precision for the weights. 1 digit of precision was tested, but this
 * gave poorer results.
 * 
 * @param playfield 
 * @return uint32_t 
 */
uint32_t ai_calculate_playfield_score(uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH]) {
    uint32_t score = 100000;
    uint8_t heights_cache[10];
    uint16_t height = ai_calculate_height(playfield, heights_cache);
    uint16_t bumpiness = ai_calculate_bumpiness(heights_cache);
    uint16_t full_lines = ai_calculate_full_lines(playfield, heights_cache);
    uint16_t holes = ai_calculate_holes(playfield, heights_cache);
    uint16_t tmp;

    // height * 510 ~= (height << 9) - (height << 1)
    score += (height << 9) - (height << 1);

    // holes * 357 ≈ 4 + 32 + 64 + 256 + 512 + 1 = 869
    tmp = holes << 2;          // holes * 4
    score += tmp;
    tmp <<= 3;                 // * 32
    score += tmp;
    tmp <<= 1;                 // * 64
    score += tmp;
    tmp <<= 2;                 // * 256
    score += tmp;
    tmp <<= 1;                 // * 512
    score += tmp;
    score += holes;            // +1

    // bumpiness * 184 ≈ 8 + 16 + 32 + 128 = 184
    tmp = bumpiness << 3;      // * 8
    score += tmp;
    tmp <<= 1;                 // * 16
    score += tmp;
    tmp <<= 1;                 // * 32
    score += tmp;
    tmp <<= 2;                 // * 128
    score += tmp;

    // full_lines * 761 ≈ 512 + 256 - 4 - 2 - 1 = 761
    tmp = full_lines << 9;     // * 512
    score -= tmp;
    tmp = full_lines << 8;     // * 256
    score -= tmp;
    tmp = full_lines << 2;     // * 4
    score += tmp;
    tmp = full_lines << 1;     // * 2
    score += tmp;
    score += full_lines;       // * 1

    return score;
}

uint8_t ai_calculate_height(uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH], 
                            uint8_t heights_cache[PLAYFIELD_WIDTH]) {
    int8_t row;
    uint8_t col;
    uint8_t height = 0;

    //print_playfield(playfield);

    for (col = 0; col < 10; col++) {
        for (row = 19; row >= 0; --row) {
            if (playfield[row][col] != 0xFF) {
                break;
            }
        }
        row++;
        heights_cache[col] = row;
        height += row;
    }

    return height;
}

uint8_t ai_calculate_bumpiness(uint8_t heights[PLAYFIELD_WIDTH]) {
    uint8_t i;
    uint8_t bumpiness = 0;

    for (i = 0; i < 9; i++) {
        int16_t diff = (int16_t)heights[i] - (int16_t)heights[i + 1];
        if (diff < 0) {
            diff = -diff;
        }
        bumpiness += (uint8_t)diff;
    }

    return bumpiness;
}

uint8_t ai_calculate_full_lines(uint8_t playfield[PLAYFIELD_HEIGHT][PLAYFIELD_WIDTH], 
                                uint8_t heights[PLAYFIELD_WIDTH]) {
    uint8_t row, col;
    uint8_t full_lines = 0;
    uint8_t is_full;
    uint8_t max_height = 0;

    // Step 1: Find the maximum height
    for (col=0; col<PLAYFIELD_WIDTH; col++) {
        if (heights[col] > max_height) {
            max_height = heights[col];
        }
    }

    // Step 2: Only check rows up to the max height
    for (row=0; row<max_height; row++) {
        is_full = 1;
        for (col=0; col<PLAYFIELD_WIDTH; col++) {
            if (playfield[row][col] == 0xFF) {
                is_full = 0;
                break;
            }
        }
        if (is_full) {
            full_lines++;
        }
    }

    return full_lines;
}

uint8_t ai_calculate_holes(uint8_t playfield[PLAYFIELD_HEIGHT][10], uint8_t heights[10]) {
    uint8_t col;
    int8_t row;
    uint8_t holes = 0;

    for (col=0; col<PLAYFIELD_WIDTH; col++) {
        uint8_t block_found = 0;
        for (row = heights[col]; row >= 0; row--) {  // top to bottom
            if (playfield[row][col] != 0xFF) {
                block_found = 1;
            } else if (block_found) {
                holes++;
            }
        }
    }

    return holes;
}
   
void print_playfield(uint8_t playfield[PLAYFIELD_HEIGHT][10]) {
    int row, col;
    for (row = 0; row < PLAYFIELD_HEIGHT; row++) {
        for (col = 0; col < PLAYFIELD_WIDTH; col++) {
            printf("%02X ", playfield[row][col]);
        }
        printf("\n");
    }
}

void ai_find_best_score(struct PlayFieldEvaluation* eval) {
    uint8_t rot, offset;
    uint32_t best_score = 0xFFFFFFFF;
    uint8_t best_rot = 0;
    uint8_t best_off = 0;

    for (rot = 0; rot < 4; rot++) {
        for (offset = 0; offset < 10; offset++) {
            uint32_t current_score = eval->score[rot][offset];
            if (current_score < best_score) {
                best_score = current_score;
                best_rot = rot;
                best_off = offset;
            }
        }
    }

    eval->best_rot_id = best_rot;
    eval->best_offset_id = best_off;
}

uint8_t ai_determine_movement(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t i;
    uint8_t leftmost_x = 255;

    // Step 1: Check if rotation is needed
    if (gamestate->piece_rot_idx != evaluation->best_rot_id) {
        return AI_ROTATE;
    }

    // Step 2: Find left-most X in piece_position
    for (i = 0; i < 4; i++) {
        if (gamestate->piece_position[i][0] < leftmost_x) {
            leftmost_x = gamestate->piece_position[i][0];
        }
    }

    // Step 3: Compare leftmost_x to best_offset_id
    if (leftmost_x > evaluation->best_offset_id) {
        return AI_MOVEMENT_LEFT;
    } else if (leftmost_x < evaluation->best_offset_id) {
        return AI_MOVEMENT_RIGHT;
    } else {
        return AI_DROP;
    }
}