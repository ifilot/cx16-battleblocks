#ifndef _AI_H
#define _AI_H

#include <string.h>
#include <ascii_charmap.h>

#include "constants.h"
#include "pieces.h"
#include "gamestate.h"

struct PlayFieldEvaluation {
    uint32_t score[4][PLAYFIELD_WIDTH];
    uint8_t pieces[4][4][2];
    uint8_t rot_id;
    uint8_t offset_id;
    uint8_t best_rot_id;
    uint8_t best_offset_id;
    uint8_t heights[PLAYFIELD_WIDTH];
    uint8_t row_counts[20];
    uint8_t widths[4];
    uint8_t unique_rotations;
    uint8_t occupied;
    uint8_t full_lines;
    uint8_t movement_stalls;
    uint32_t best_score;
    /* Four-candidate beam, expanded one preview placement per game-loop turn. */
    uint16_t rows[20];
    uint16_t future_rows[20];
    uint8_t future_heights[10];
    uint8_t future_counts[20];
    uint8_t future_occupied;
    uint8_t candidates[4];
    uint8_t candidate_count;
    uint8_t candidate_id;
    uint8_t phase;
    uint8_t next_type;
    uint8_t next_rot;
    uint8_t next_offset;
    uint32_t immediate_score;
    uint32_t future_score;
};

extern struct PlayFieldEvaluation pfeval;
extern struct PlayFieldEvaluation pfeval_player1;

void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

uint8_t ai_calculate_next_score(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

uint8_t ai_determine_movement(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);


#endif // _AI_H
