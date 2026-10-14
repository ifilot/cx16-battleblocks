; SNES events through KERNAL joystick_get, excluding keyboard joystick.
; Directions repeat after 12 ticks, then every 3; buttons use debounced edges.
.export _input_controller, _input_key, _input_pause
.export _game_tick_offset, _game_paused
.import _cbm_k_getin
.import _menu_quit
.export _input_freeze_begin, _input_freeze_tick
.bss
previous: .res 2
deadline: .res 2
port: .res 1
held: .res 1
tick: .res 1
button_sample: .res 2
button_previous: .res 2
button_older: .res 2
sample_tick: .res 2
_game_tick_offset: .res 1
_game_paused: .res 1
pause_tick: .res 1
.rodata
keys: .byte $1d, $9d, $11, $91, $0d, $1b, $20, $91
.code
.proc _input_controller
    sta port
    jsr $ff56
    cpy #0
    beq connected
    lda #0
    bra normalized
connected:
    eor #$ff
    sta held
    and #$c0
    beq no_rotate
    lda held
    ora #$80
    sta held
no_rotate:
    txa
    eor #$ff
    and #$c0
    beq no_drop
    lda held
    ora #$40
    bra normalized
no_drop:
    lda held
    and #$bf
normalized:
    sta held
    jsr $ffde              ; Controller repeats use real time, also while paused.
    sta tick
    ldx port
    dex
    ; Remember buttons seen in this and the previous two 60 Hz ticks. A press is
    ; immediate, but a brief release cannot re-arm it, even within one tick.
    cmp sample_tick,x
    beq sampled
    sta sample_tick,x
    lda button_previous,x
    sta button_older,x
    lda button_sample,x
    sta button_previous,x
    stz button_sample,x
sampled:
    lda held
    and #$f0
    ora button_sample,x
    sta button_sample,x
    ora button_previous,x
    ora button_older,x
    ora held
    sta held
    lda held
    eor previous,x
    and held
    tay
    lda held
    cmp previous,x
    sta previous,x
    beq unchanged
    lda tick
    clc
    adc #12
    sta deadline,x
    bra events
unchanged:
    lda tick
    sec
    sbc deadline,x
    bmi events
    lda tick
    clc
    adc #3
    sta deadline,x
    lda held
    and #$0f
    tay
events:
    tya
    ldy #7
find:
    asl
    bcs found
    dey
    bpl find
    lda #0
    tax
    rts
found:
    lda keys,y
    ldx #0
    rts
.endproc

; Stop match time while polling both controllers. Movement is ignored; Escape or
; Select opens the quit dialog. Return one for confirmed quitting, zero for a
; fresh Start press on either port.
.proc _input_pause
    inc _game_paused
    ; Consume simultaneous presses on both ports before waiting for a new edge.
    ; Either port may have triggered pause between the other port's polls.
    lda #1
    jsr _input_controller
    lda #2
    jsr _input_controller
    jsr _input_freeze_begin
loop:
    jsr _input_freeze_tick
    jsr _cbm_k_getin
    cmp #$1b
    beq quit
    lda #1
    jsr _input_controller
    pha
    lda #2
    jsr _input_controller
    tay
    pla
    cmp #$0d
    beq resume
    cpy #$0d
    beq resume
    cmp #$1b
    beq quit
    cpy #$1b
    bne loop
quit:
    jsr _menu_quit
    cmp #0
    beq loop
    lda #1
    bra done
resume:
    lda #0
done:
    stz _game_paused
    ldx #0
    rts
.endproc

.proc _input_freeze_begin
    jsr $ffde
    sta pause_tick
    rts
.endproc

.proc _input_freeze_tick
    jsr $ffde
    pha
    sec
    sbc pause_tick
    clc
    adc _game_tick_offset
    sta _game_tick_offset
    pla
    sta pause_tick
    rts
.endproc

; Poll both ports even when the keyboard or the first port wins priority.
.proc _input_key
    jsr _cbm_k_getin
    pha
    lda #1
    jsr _input_controller
    pha
    lda #2
    jsr _input_controller
    tay
    pla
    bne controller
    tya
controller:
    tay
    pla
    bne done
    tya
done:
    ldx #0
    rts
.endproc
