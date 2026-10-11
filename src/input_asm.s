; SNES events through KERNAL joystick_get, excluding keyboard joystick.
; Directions repeat after 12 ticks, then every 3; buttons use press edges.
.export _input_controller, _input_key
.import _game_get_tick, _cbm_k_getin
.bss
previous: .res 2
deadline: .res 2
port: .res 1
held: .res 1
tick: .res 1
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
    jsr _game_get_tick
    sta tick
    ldx port
    dex
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
