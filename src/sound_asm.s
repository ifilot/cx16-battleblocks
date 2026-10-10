; ZSMKit v1 playback from RAM; music priority 0, P1/menu 1, P2 2.
; Engine state owns bank 1, effect data bank 2. Preserve the caller's bank.
.scope zsmkit
.include "zsmkit.inc"
.endscope

.export _sound_engine_init, _sound_play_music, _sound_play_effect
.export _sound_stop_effects, _sfx_prio
.export _sound_load_file
.export _sound_music_atten, _sound_effects_atten

.import __BSS_RUN__, __BSS_SIZE__, __HIMEM__, __STACKSIZE__
.assert __BSS_RUN__ + __BSS_SIZE__ <= __HIMEM__ - __STACKSIZE__, lderror, "Game data overlaps the software stack or I/O"

.bss
_sfx_prio: .res 1
saved_bank: .res 1
music_bank: .res 1
effects_atten: .res 1

.code
; SETNAM/SETLFS and the starting RAM bank are already selected by C.
; Check KERNAL's carry result; an unchanged end address is not a success flag.
.proc _sound_load_file
    lda #0
    ldx #<$A000
    ldy #>$A000
    jsr $FFD5
    bcs failed
    lda #1
    ldx #0
    rts
failed:
    lda #0
    tax
    rts
.endproc

.proc _sound_engine_init
    php
    sei
    lda $00
    pha
    lda #1
    jsr zsmkit::zsm_init_engine
    jsr zsmkit::zsmkit_setisr
    pla
    sta $00
    plp
    rts
.endproc

.proc _sound_play_music
    php
    sei
    sta music_bank
    lda $00
    sta saved_bank
    lda music_bank
    sta $00
    lda #<$A000
    ldy #>$A000
    ldx #0
    jsr zsmkit::zsm_setmem
    ldx #0
    jsr zsmkit::zsm_play
    lda saved_bank
    sta $00
    plp
    rts
.endproc

.proc _sound_play_effect
    php
    sei
    clc
    adc #<$A000
    pha
    txa
    adc #>$A000
    tay
    lda $00
    sta saved_bank
    lda #2
    sta $00
    pla
    ldx _sfx_prio
    jsr zsmkit::zsm_setmem
    ldx _sfx_prio
    clc
    jsr zsmkit::zsm_setloop
    ldx _sfx_prio
    jsr zsmkit::zsm_play
    lda saved_bank
    sta $00
    plp
    rts
.endproc

.proc _sound_stop_effects
    php
    sei
    ldx #1
    jsr zsmkit::zsm_stop
    ldx #2
    jsr zsmkit::zsm_stop
    plp
    rts
.endproc

; fastcall A = attenuation (0 full volume, 63 mute).
; Music owns priority 0; both players and the menu use priorities 1/2.
.proc _sound_music_atten
    php
    sei
    ldx #0
    jsr zsmkit::zsm_setatten
    plp
    rts
.endproc

.proc _sound_effects_atten
    php
    sei
    sta effects_atten
    ldx #1
    jsr zsmkit::zsm_setatten
    lda effects_atten
    ldx #2
    jsr zsmkit::zsm_setatten
    plp
    rts
.endproc
