#include "sound.h"
#include "game.h"
#include <cbm.h>

#define RAM_BANK (*(volatile uint8_t*)0x00)

uint8_t sound_ready;
uint8_t sound_current_music = 255;
static uint8_t next_battle;

extern void sound_engine_init(void);
extern void sound_stop_effects(void);
extern uint8_t sound_load_file(void);
extern void __fastcall__ sound_play_music(uint8_t bank);
extern void __fastcall__ sound_play_effect(uint16_t offset);

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
}

static void sound_music(uint8_t bank) {
    if(!sound_ready || sound_current_music == bank) return;
    sound_play_music(bank);
    sound_current_music = bank;
}

void sound_title(void) {
    if(sound_current_music == MUSIC_TITLE_BANK) return;
    if(sound_ready) sound_stop_effects();
    sound_music(MUSIC_TITLE_BANK);
}

void sound_battle(void) {
    static const uint8_t banks[4] = {MUSIC_BATTLE_BANK, MUSIC_BATTLE2_BANK,
                                    MUSIC_BATTLE3_BANK, MUSIC_BATTLE4_BANK};
    if(sound_ready) sound_stop_effects();
    sound_music(banks[next_battle]);
    next_battle = (next_battle + 1) & 3;
}

void sound_result(uint8_t victory) {
    if(sound_ready) sound_stop_effects();
    sound_music(victory ? MUSIC_VICTORY_BANK : MUSIC_GAMEOVER_BANK);
}

void sound_menu_effect(uint16_t effect) {
    if(!sound_ready) return;
    sfx_prio = 1;
    sound_play_effect(effect);
}

void sound_player_effect(uint8_t player, uint16_t p1, uint16_t p2) {
    if(!sound_ready) return;
    sfx_prio = player == PLAYER1 ? 1 : 2;
    sound_play_effect(player == PLAYER1 ? p1 : p2);
}

void sound_player_motion(uint8_t player, uint16_t p1, uint16_t p2) {
    if(game_mode == GAME_MODE_DEMO ||
       (game_mode == GAME_MODE_COMPUTER && player == PLAYER2)) return;
    sound_player_effect(player, p1, p2);
}

void sound_clear(uint8_t player, uint8_t count) {
    static const uint16_t p1[4] = {SFX_CLEAR1_P1, SFX_CLEAR2_P1, SFX_CLEAR3_P1, SFX_CLEAR4_P1};
    static const uint16_t p2[4] = {SFX_CLEAR1_P2, SFX_CLEAR2_P2, SFX_CLEAR3_P2, SFX_CLEAR4_P2};
    if(!count) return;
    if(count > 4) count = 4;
    sound_player_effect(player, p1[count - 1], p2[count - 1]);
}
