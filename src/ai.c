#include "ai.h"

struct PlayFieldEvaluation pfeval;
struct PlayFieldEvaluation pfeval_player1;

static const uint16_t column_bits[10] = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512};

void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t row, col;
    uint8_t type = gamestate->piece_type;
    const uint8_t* cell = &gamestate->playfield[0][0];
    evaluation->unique_rotations = pieces_unique_rotations[type];
    memcpy(evaluation->pieces, pieces_normalized[type], sizeof(evaluation->pieces));
    memcpy(evaluation->widths, pieces_widths[type], sizeof(evaluation->widths));
    memset(evaluation->score, 0xFF, sizeof(evaluation->score));
    memset(evaluation->heights, 0, sizeof(evaluation->heights));
    memset(evaluation->row_counts, 0, sizeof(evaluation->row_counts));
    memset(evaluation->rows, 0, sizeof(evaluation->rows));
    evaluation->occupied = 0;
    evaluation->full_lines = 0;
    for(row = 0; row < 20; row++) {
        for(col = 0; col < PLAYFIELD_WIDTH; col++) {
            if(*cell++ != 0xFF) {
                evaluation->rows[row] |= column_bits[col];
                evaluation->heights[col] = row + 1;
                evaluation->row_counts[row]++;
                evaluation->occupied++;
            }
        }
        if(evaluation->row_counts[row] == PLAYFIELD_WIDTH) evaluation->full_lines++;
    }

    // reset evaluation
    evaluation->rot_id = 0;
    evaluation->offset_id = 0;
    evaluation->best_rot_id = 0xFF;
    evaluation->best_offset_id = 0xFF;
    evaluation->best_score = 0xFFFFFFFFUL;
    evaluation->movement_stalls = 0;
    evaluation->candidate_count = 0;
    evaluation->candidate_id = 0;
    evaluation->phase = 0;
    evaluation->next_type = gamestate->piece_list.bag1.pieces[gamestate->piece_list.bag1.idx];
}

/* Integer heuristic, with wide intermediates: tall stacks must not wrap at
 * 65536. Keep the existing weights and nonlinear penalty for deep wells.
 */
static uint32_t ai_score(uint16_t height, uint16_t bumpiness,
                         uint16_t holes, uint8_t full_lines) {
    uint32_t h = height, b = bumpiness, o = holes, f = full_lines;
    return 100000UL + (h << 9) - (h << 1)
        + (o << 9) + (o << 8) + (o << 6) + (o << 5) + (o << 2) + o
        + (b << 7) + (b << 5) + (b << 4) + (b << 3)
        - (f << 9) - (f << 8) + (f << 2) + (f << 1) + f;
}

static const uint8_t bumpweights[] = {0, 1, 2, 3, 4, 8, 16, 32, 64, 75, 100, 125};

static uint8_t ai_landing(const uint8_t* heights, const uint8_t shape[4][2],
                          uint8_t offset) {
    uint8_t i, landing = 0;
    int8_t required;
    for(i = 0; i < 4; i++) {
        required = heights[shape[i][0] + offset] - shape[i][1];
        if(required > (int8_t)landing) landing = required;
    }
    return landing;
}

/* Collapse full rows and rebuild the profile without changing the live board. */
static uint8_t ai_profile(uint16_t* rows, uint8_t* heights, uint8_t* counts,
                          uint8_t* occupied) {
    uint8_t row, dest = 0, col, lines = 0;
    uint16_t mask;
    memset(heights, 0, 10);
    memset(counts, 0, 20);
    *occupied = 0;
    for(row = 0; row < 20; row++) {
        mask = rows[row];
        if(mask == 0x03FF) { lines++; continue; }
        rows[dest] = mask;
        if(!mask) { dest++; continue; }
        for(col = 0; col < 10; col++) {
            if(mask & 1) {
                heights[col] = dest + 1;
                counts[dest]++;
                (*occupied)++;
            }
            mask >>= 1;
        }
        dest++;
    }
    while(dest < 20) rows[dest++] = 0;
    return lines;
}

static uint32_t ai_profile_score(const uint8_t* heights, uint8_t occupied,
                                 uint8_t lines) {
    uint8_t i;
    int8_t diff;
    uint16_t height = 0, bumpiness = 0;
    for(i = 0; i < 10; i++) {
        height += heights[i];
        if(i) {
            diff = heights[i] - heights[i - 1];
            if(diff < 0) diff = -diff;
            bumpiness += bumpweights[diff > 11 ? 11 : diff];
        }
    }
    return ai_score(height, bumpiness, height - occupied, lines);
}

static uint32_t ai_placement_score(const uint16_t* rows, const uint8_t* base_heights,
                                   const uint8_t* base_counts, uint8_t occupied,
                                   const uint8_t shape[4][2], uint8_t offset, uint8_t lines) {
    uint8_t heights[10], counts[20], i, x, y;
    uint16_t placed[20];
    uint8_t landing = ai_landing(base_heights, shape, offset);
    memcpy(heights, base_heights, 10);
    memcpy(counts, base_counts, 20);
    for(i = 0; i < 4; i++) {
        x = shape[i][0] + offset;
        y = shape[i][1] + landing;
        if(y >= 20) return 0xFFFFFFFFUL;
        if(y + 1 > heights[x]) heights[x] = y + 1;
        if(++counts[y] == 10) lines++;
    }
    occupied += 4;
    if(lines) {
        memcpy(placed, rows, sizeof(placed));
        for(i = 0; i < 4; i++) {
            placed[shape[i][1] + landing] |= column_bits[shape[i][0] + offset];
        }
        ai_profile(placed, heights, counts, &occupied);
    }
    return ai_profile_score(heights, occupied, lines);
}

/* Keep the four best immediate placements, preserving deterministic ties. */
static void ai_shortlist(struct PlayFieldEvaluation* e, uint8_t candidate,
                          uint32_t score) {
    uint8_t i, j;
    for(i = 0; i < e->candidate_count; i++) {
        j = e->candidates[i];
        if(score < e->score[j / 10][j % 10]) break;
    }
    if(i == 4) return;
    if(e->candidate_count < 4) e->candidate_count++;
    for(j = e->candidate_count - 1; j > i; j--) e->candidates[j] = e->candidates[j - 1];
    e->candidates[i] = candidate;
}

static void ai_prepare_future(struct PlayFieldEvaluation* e) {
    uint8_t candidate = e->candidates[e->candidate_id];
    uint8_t rot = candidate / 10, offset = candidate % 10, i, landing, lines;
    memcpy(e->future_rows, e->rows, sizeof(e->rows));
    landing = ai_landing(e->heights, e->pieces[rot], offset);
    for(i = 0; i < 4; i++) {
        e->future_rows[e->pieces[rot][i][1] + landing] |=
            column_bits[e->pieces[rot][i][0] + offset];
    }
    lines = ai_profile(e->future_rows, e->future_heights, e->future_counts,
                       &e->future_occupied);
    e->immediate_score = ai_profile_score(e->future_heights, e->future_occupied, lines);
    e->future_score = 0xFFFFFFFFUL;
    e->next_rot = 0;
    e->next_offset = 0;
}

uint8_t ai_calculate_next_score(struct GameState* gamestate,
                                struct PlayFieldEvaluation* e) {
    uint8_t rot, offset;
    uint32_t score;
    (void)gamestate; // Both searches use immutable snapshots of the board.
    if(e->phase == 2) return 1;
    if(e->phase == 0) {
        rot = e->rot_id;
        offset = e->offset_id;
        score = ai_placement_score(e->rows, e->heights, e->row_counts, e->occupied,
                                   e->pieces[rot], offset, e->full_lines);
        e->score[rot][offset] = score;
        if(score != 0xFFFFFFFFUL) ai_shortlist(e, rot * 10 + offset, score);
        if(++e->offset_id > PLAYFIELD_WIDTH - e->widths[rot]) {
            e->offset_id = 0;
            do { e->rot_id++; }
            while(e->rot_id < 4 && !(e->unique_rotations & (1 << e->rot_id)));
        }
        if(e->rot_id < 4) return 0;
        if(!e->candidate_count) { e->phase = 2; return 1; }
        e->phase = 1;
        // Start steering while preview search runs; gravity can be fast.
        offset = e->candidates[0];
        e->best_rot_id = offset / 10;
        e->best_offset_id = offset % 10;
        // Preparing a simulated board is a separate, bounded game-loop step.
        e->next_rot = 4;
        return 0;
    }
    if(e->next_rot == 4) { ai_prepare_future(e); return 0; }
    rot = e->next_rot;
    offset = e->next_offset;
    score = ai_placement_score(e->future_rows, e->future_heights, e->future_counts,
                               e->future_occupied, pieces_normalized[e->next_type][rot], offset, 0);
    if(score < e->future_score) e->future_score = score;
    if(++e->next_offset > PLAYFIELD_WIDTH - pieces_widths[e->next_type][rot]) {
        e->next_offset = 0;
        do { e->next_rot++; }
        while(e->next_rot < 4 && !(pieces_unique_rotations[e->next_type] & (1 << e->next_rot)));
    }
    if(e->next_rot < 4) return 0;
    // A doomed preview must lose to every candidate with a legal continuation.
    score = e->immediate_score + (e->future_score == 0xFFFFFFFFUL ? 1000000UL : e->future_score);
    if(score < e->best_score) {
        e->best_score = score;
        offset = e->candidates[e->candidate_id];
        if(e->best_rot_id != offset / 10 || e->best_offset_id != offset % 10) {
            e->movement_stalls = 0;
        }
        e->best_rot_id = offset / 10;
        e->best_offset_id = offset % 10;
    }
    if(++e->candidate_id == e->candidate_count) { e->phase = 2; return 1; }
    return 0;
}

uint8_t ai_determine_movement(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation) {
    uint8_t i;
    uint8_t leftmost_x = 255;

    // No visible landing exists. Lock the current piece and finish the match
    // rather than endlessly steering towards an invalid candidate.
    if(evaluation->best_rot_id == 0xFF) return AI_DROP;

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
