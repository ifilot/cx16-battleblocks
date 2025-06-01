#include "ai.h"

struct PlayFieldEvaluation pfeval;

void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t i,j;
    uint8_t piece[4][2];
    uint8_t miny = 20;
    uint8_t minx = 20;
    uint8_t rot_id = 0;
    uint8_t type = gamestate->piece_type;

    printf("Preparing new evaluation\n");

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

uint8_t ai_calculate_next_score(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t i;
    int8_t row;
    uint8_t x,y;
    uint8_t rot_id = evaluation->rot_id;
    uint8_t offset_id = evaluation->offset_id;
    uint8_t hit = 0;
    uint8_t playfield[22][10];
    uint8_t noeval = 0;

    if(rot_id >= 4) {
        if(evaluation->best_rot_id == 0xFF) {
            ai_find_best_score(evaluation);
            printf("Best option: %U %U\n", evaluation->best_rot_id, evaluation->best_offset_id);
        }
        return 1;
    }

    // create a copy of current play field
    memcpy(playfield, gamestate->playfield, 220);

    // find lowest position piece can be placed
    for(row=18; row>=0; row--) {
        hit = 1;
        for(i=0; i<4; i++) {
            x = evaluation->pieces[rot_id][i][0] + offset_id;
            y = row + evaluation->pieces[rot_id][i][1];
            if(gamestate->playfield[y][x] != 0xFF) {
                hit = 0;
                break;
            }
        }

        if(hit == 1 || row == 0) {
            if(hit == 1) {
                row++;
            }
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
            break;
        }
    }

    if(noeval == 1) {
        evaluation->score[rot_id][offset_id] = -10000;
    } else {
        evaluation->score[rot_id][offset_id] = ai_calculate_playfield_score(playfield);
    }

    printf("Score: %I %I %I\n", rot_id, offset_id, evaluation->score[rot_id][offset_id]);

    if((evaluation->offset_id++) == 9) {
        evaluation->rot_id++;
        evaluation->offset_id = 0;
    }

    return 0;
}

int16_t ai_calculate_playfield_score(uint8_t playfield[22][10]) {
    int16_t score = 0;
    uint8_t heights_cache[10];
    uint8_t height = ai_calculate_height(playfield, heights_cache);
    uint8_t bumpiness = ai_calculate_bumpiness(heights_cache);
    uint8_t full_lines = ai_calculate_full_lines(playfield, heights_cache);
    uint8_t holes = ai_calculate_holes(playfield, heights_cache);

    // -5 * height = (-4 * height) - height
    score -= (height << 2); // -4×
    score -= height;        // -1×

    // +10 * lines = (8 * lines) + (2 * lines)
    score += (full_lines << 3); // 8×
    score += (full_lines << 1); // 2×

    // -8 * holes
    score -= (holes << 3);

    // -3 * bumpiness = (-2 * bumpiness) - bumpiness
    score -= (bumpiness << 1);
    score -= bumpiness;

    return score;
}

uint8_t ai_calculate_height(uint8_t playfield[22][10], uint8_t heights_cache[10]) {
    int8_t i;
    uint8_t j;
    uint8_t counter = 0;

    for (j = 0; j < 10; j++) {
        for (i = 19; i >= 0; --i) {
            if (playfield[i][j] != 0xFF) {
                break;
            }
        }
        heights_cache[j] = i + 1;
        counter += heights_cache[j];
    }

    return counter;
}

uint8_t ai_calculate_bumpiness(uint8_t heights[10]) {
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

uint8_t ai_calculate_full_lines(uint8_t playfield[22][10], uint8_t heights[10]) {
    uint8_t row, col;
    uint8_t full_lines = 0;
    uint8_t is_full;
    uint8_t max_height = 0;

    // Step 1: Find the maximum height
    for (col = 0; col < 10; col++) {
        if (heights[col] > max_height) {
            max_height = heights[col];
        }
    }

    // Step 2: Only check rows up to the max height
    for (row = 0; row < max_height; row++) {
        is_full = 1;
        for (col = 0; col < 10; col++) {
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

uint8_t ai_calculate_holes(uint8_t playfield[22][10], uint8_t heights[10]) {
    uint8_t col;
    int8_t row;
    uint8_t holes = 0;

    for (col = 0; col < 10; col++) {
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
   
void print_playfield(uint8_t playfield[22][10]) {
    int row, col;
    for (row = 0; row < 22; row++) {
        for (col = 0; col < 10; col++) {
            printf("%02X ", playfield[row][col]);
        }
        printf("\n");
    }
}

void ai_find_best_score(struct PlayFieldEvaluation* eval) {
    uint8_t rot, offset;
    int16_t best_score = -10000;
    uint8_t best_rot = 0;
    uint8_t best_off = 0;

    for (rot = 0; rot < 4; rot++) {
        for (offset = 0; offset < 10; offset++) {
            int16_t current_score = eval->score[rot][offset];
            printf("%U %U %I\n", rot, offset, current_score);
            if (current_score > best_score) {
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
        return AI_MOVEMENT_DOWN;
    }
}