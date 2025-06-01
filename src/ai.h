#ifndef _AI_H
#define _AI_H

#include <stdio.h>
#include <string.h>
#include <ascii_charmap.h>

#include "constants.h"
#include "pieces.h"
#include "gamestate.h"

struct PlayFieldEvaluation {
    int16_t score[4][10];
    uint8_t pieces[4][4][2];
    uint8_t rot_id;
    uint8_t offset_id;
    uint8_t best_rot_id;
    uint8_t best_offset_id;
};

extern struct PlayFieldEvaluation pfeval;

void ai_prepare(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

uint8_t ai_calculate_next_score(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

int16_t ai_calculate_playfield_score(uint8_t playfield[22][10]);

uint8_t ai_calculate_height(uint8_t playfield[22][10], uint8_t heights_cache[10]);
uint8_t ai_calculate_bumpiness(uint8_t heights[10]);
uint8_t ai_calculate_full_lines(uint8_t playfield[22][10], uint8_t heights[10]);
uint8_t ai_calculate_holes(uint8_t playfield[22][10], uint8_t heights[10]);
void ai_find_best_score(struct PlayFieldEvaluation* evaluation);
uint8_t ai_determine_movement(struct GameState* gamestate, struct PlayFieldEvaluation* evaluation);

void print_playfield(uint8_t playfield[22][10]);

#endif // _AI_H