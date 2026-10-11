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

#include "sound.h"
#include "game.h"
#include <cbm.h>

/*
 * Music and player effects backed by ZSMKit. Audio assets load once into
 * banked RAM; playback and channel volumes are controlled during the game.
 */

#define RAM_BANK (*(volatile uint8_t*)0x00)

uint8_t sound_ready;
uint8_t sound_current_music = 255;
uint8_t sound_music_enabled = 1;
uint8_t sound_effects_enabled = 1;
uint8_t sound_music_volume = 9;
uint8_t sound_effects_volume = 10;
static uint8_t next_battle;

/**
 * @brief Initialize the assembly audio engine and playback interrupt
 */
extern void sound_engine_init(void);

/**
 * @brief Stop active effects before changing scenes
 */
extern void sound_stop_effects(void);

/**
 * @brief Load into the selected RAM bank; return one on success
 */
extern uint8_t sound_load_file(void);

/**
 * @brief Start a music stream from the supplied RAM bank
 */
extern void __fastcall__ sound_play_music(uint8_t bank);

/**
 * @brief Start an effect at the supplied offset using sfx_prio
 */
extern void __fastcall__ sound_play_effect(uint16_t offset);

/**
 * @brief Set music attenuation, 0 (full volume) through 63 (silent)
 */
extern void __fastcall__ sound_music_atten(uint8_t attenuation);

/**
 * @brief Set effect attenuation, 0 (full volume) through 63 (silent)
 */
extern void __fastcall__ sound_effects_atten(uint8_t attenuation);

/**
 * @brief Convert a channel's enable flag and 0..10 volume to PSG attenuation
 */
static uint8_t attenuation(uint8_t enabled, uint8_t volume) {
    if(!enabled || !volume) return 63;
    return (10 - volume) * 4;
}

/**
 * @brief Apply channel enable flags and volume levels to the audio engine
 */
void sound_apply_options(void) {
    if(!sound_ready) return;
    sound_music_atten(attenuation(sound_music_enabled, sound_music_volume));
    sound_effects_atten(attenuation(sound_effects_enabled, sound_effects_volume));
}

/**
 * @brief Load a headerless audio file while preserving the caller's RAM bank
 */
static uint8_t load_banked(const char* name, uint8_t bank) {
    uint8_t saved = RAM_BANK;
    uint8_t loaded;
    RAM_BANK = bank;
    cbm_k_setnam(name);
    cbm_k_setlfs(0, 8, 2); /* Headerless; LOAD continues across RAM banks. */
    loaded = sound_load_file();
    RAM_BANK = saved;
    return loaded;
}

/**
 * @brief Load audio assets and install playback; leave sound_ready clear on failure
 */
void sound_init(void) {
    uint8_t i;
    static const char* const files[] = {MUSIC_TITLE_FILE, MUSIC_BATTLE_FILE,
        MUSIC_BATTLE2_FILE, MUSIC_BATTLE3_FILE, MUSIC_BATTLE4_FILE,
        MUSIC_VICTORY_FILE, MUSIC_GAMEOVER_FILE};
    static const uint8_t banks[] = {MUSIC_TITLE_BANK, MUSIC_BATTLE_BANK,
        MUSIC_BATTLE2_BANK, MUSIC_BATTLE3_BANK, MUSIC_BATTLE4_BANK,
        MUSIC_VICTORY_BANK, MUSIC_GAMEOVER_BANK};
    /* Load once before installing the ISR. Gameplay never opens audio files. */
    if(!load_banked(SFX_BANK_FILE, 2)) return;
    for(i = 0; i < sizeof(banks); i++)
        if(!load_banked(files[i], banks[i])) return;
    sound_engine_init();
    sound_ready = 1;
    sound_apply_options();
}

/**
 * @brief Change tracks only when audio is ready and the bank differs
 */
static void sound_music(uint8_t bank) {
    if(!sound_ready || sound_current_music == bank) return;
    sound_play_music(bank);
    sound_current_music = bank;
}

/**
 * @brief Start title music and stop lingering effects when changing tracks
 */
void sound_title(void) {
    if(sound_current_music == MUSIC_TITLE_BANK) return;
    if(sound_ready) sound_stop_effects();
    sound_music(MUSIC_TITLE_BANK);
}

/**
 * @brief Start the next of four rotating battle tracks and stop old effects
 */
void sound_battle(void) {
    static const uint8_t banks[4] = {MUSIC_BATTLE_BANK, MUSIC_BATTLE2_BANK,
                                    MUSIC_BATTLE3_BANK, MUSIC_BATTLE4_BANK};
    if(sound_ready) sound_stop_effects();
    sound_music(banks[next_battle]);
    next_battle = (next_battle + 1) & 3;
}

/**
 * @brief Start victory or game-over music and stop old effects
 *
 * @param victory Nonzero for victory music; zero for game-over music
 */
void sound_result(uint8_t victory) {
    if(sound_ready) sound_stop_effects();
    sound_music(victory ? MUSIC_VICTORY_BANK : MUSIC_GAMEOVER_BANK);
}

/**
 * @brief Play a menu effect using player-one priority
 *
 * @param effect SFX_* byte offset in the effect bank
 */
void sound_menu_effect(uint16_t effect) {
    if(!sound_ready || !sound_effects_enabled || !sound_effects_volume) return;
    sfx_prio = 1;
    sound_play_effect(effect);
}

/**
 * @brief Play the effect for the requested player with its channel priority
 *
 * @param player PLAYER1 or PLAYER2
 * @param p1     Player-one SFX_* byte offset
 * @param p2     Player-two SFX_* byte offset
 */
void sound_player_effect(uint8_t player, uint16_t p1, uint16_t p2) {
    if(!sound_ready || !sound_effects_enabled || !sound_effects_volume) return;
    sfx_prio = player == PLAYER1 ? 1 : 2;
    sound_play_effect(player == PLAYER1 ? p1 : p2);
}

/**
 * @brief Play a movement effect for human players only
 *
 * @param player PLAYER1 or PLAYER2
 * @param p1     Player-one SFX_* byte offset
 * @param p2     Player-two SFX_* byte offset
 */
void sound_player_motion(uint8_t player, uint16_t p1, uint16_t p2) {
    if(game_mode == GAME_MODE_DEMO ||
       (game_mode == GAME_MODE_COMPUTER && player == PLAYER2)) return;
    sound_player_effect(player, p1, p2);
}

/**
 * @brief Play the player's effect for clearing one to four rows
 *
 * @param player PLAYER1 or PLAYER2
 * @param count  Cleared rows; zero is silent and values above four are capped
 */
void sound_clear(uint8_t player, uint8_t count) {
    static const uint16_t p1[4] = {SFX_CLEAR1_P1, SFX_CLEAR2_P1, SFX_CLEAR3_P1, SFX_CLEAR4_P1};
    static const uint16_t p2[4] = {SFX_CLEAR1_P2, SFX_CLEAR2_P2, SFX_CLEAR3_P2, SFX_CLEAR4_P2};
    if(!count) return;
    if(count > 4) count = 4;
    sound_player_effect(player, p1[count - 1], p2[count - 1]);
}
