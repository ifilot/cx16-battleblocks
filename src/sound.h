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

#ifndef BATTLEBLOCKS_SOUND_H
#define BATTLEBLOCKS_SOUND_H

#include <stdint.h>
#include "sfx.h"
#include "music.h"

/*
 * Music and player effects backed by ZSMKit. Audio assets load once into
 * banked RAM; playback and channel volumes are controlled during the game.
 */

extern uint8_t sound_ready;                      // Nonzero after all assets and the audio engine load
extern uint8_t sound_current_music;              // Current music bank; 255 before any track starts
extern uint8_t sfx_prio;                         // Effect priority: music 0, player one/menu 1, player two 2
extern uint8_t sound_music_enabled;              // Music enable flag for this session
extern uint8_t sound_effects_enabled;            // Effects enable flag for this session
extern uint8_t sound_music_volume;               // Music level, 0..10; disabling preserves this value
extern uint8_t sound_effects_volume;             // Effects level, 0..10; disabling preserves this value

/**
 * @brief Apply channel enable flags and volume levels to the audio engine
 *
 * Separate 0-10 levels; disabling a channel preserves its chosen volume.
 */
void sound_apply_options(void);

/**
 * @brief Load audio assets and install playback; leave sound_ready clear on failure
 */
void sound_init(void);

/**
 * @brief Start title music and stop lingering effects when changing tracks
 */
void sound_title(void);

/**
 * @brief Start the next of four rotating battle tracks and stop old effects
 */
void sound_battle(void);

/**
 * @brief Start victory or game-over music and stop old effects
 *
 * @param victory Nonzero for victory music; zero for game-over music
 */
void sound_result(uint8_t victory);

/**
 * @brief Play a menu effect using player-one priority
 *
 * @param effect SFX_* byte offset in the effect bank
 */
void sound_menu_effect(uint16_t effect);

/**
 * @brief Play the effect for the requested player with its channel priority
 *
 * @param player PLAYER1 or PLAYER2
 * @param p1     Player-one SFX_* byte offset
 * @param p2     Player-two SFX_* byte offset
 */
void sound_player_effect(uint8_t player, uint16_t p1, uint16_t p2);

/**
 * @brief Play a movement effect for human players only
 *
 * @param player PLAYER1 or PLAYER2
 * @param p1     Player-one SFX_* byte offset
 * @param p2     Player-two SFX_* byte offset
 */
void sound_player_motion(uint8_t player, uint16_t p1, uint16_t p2);

/**
 * @brief Play the player's effect for clearing one to four rows
 *
 * @param player PLAYER1 or PLAYER2
 * @param count  Cleared rows; zero is silent and values above four are capped
 */
void sound_clear(uint8_t player, uint8_t count);

/**
 * @brief Stop all music and effects and restore the KERNAL's IRQ handler
 */
void sound_shutdown(void);

#define PLAYER_SOUND(gs, name) sound_player_effect((gs)->player, name##_P1, name##_P2)
#define MOTION_SOUND(gs, name) sound_player_motion((gs)->player, name##_P1, name##_P2)

#endif // BATTLEBLOCKS_SOUND_H
