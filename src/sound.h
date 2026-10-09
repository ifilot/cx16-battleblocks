#ifndef BATTLEBLOCKS_SOUND_H
#define BATTLEBLOCKS_SOUND_H

#include <stdint.h>
#include "sfx.h"
#include "music.h"

extern uint8_t sound_ready;
extern uint8_t sound_current_music;
extern uint8_t sfx_prio;

void sound_init(void);
void sound_title(void);
void sound_battle(void);
void sound_result(uint8_t victory);
void sound_menu_effect(uint16_t effect);
void sound_player_effect(uint8_t player, uint16_t p1, uint16_t p2);
void sound_player_motion(uint8_t player, uint16_t p1, uint16_t p2);
void sound_clear(uint8_t player, uint8_t count);

#define PLAYER_SOUND(gs, name) sound_player_effect((gs)->player, name##_P1, name##_P2)
#define MOTION_SOUND(gs, name) sound_player_motion((gs)->player, name##_P1, name##_P2)

#endif
