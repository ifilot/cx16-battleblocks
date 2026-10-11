; Resident loading label using the selected scene palette.
.include "vera.inc"
.include "constants.inc"
.include "zeropage.inc"
.export _screen_transition_init, _screen_transition_begin, _screen_transition_end
.export _screen_loading_begin
.import _screen_style

.code
.proc _screen_transition_init
    stz VERA_ADDR_CTRL
    lda #$5c               ; Unused Commander X16 white at palette index 46
    sta VERA_ADDR_LOW
    lda #$fa
    sta VERA_ADDR_HIGH
    lda #$11
    sta VERA_ADDR_BANK
    lda #$ff
    sta VERA_DATA0
    lda #$0f
    sta VERA_DATA0
    lda #$7c               ; Unused Cyberpunk cyan at palette index 62
    sta VERA_ADDR_LOW
    lda #$ef
    sta VERA_DATA0
    stz VERA_DATA0
    ; Disable all sprites before preparing the resident loading label.
    stz VERA_ADDR_LOW
    lda #$fc
    sta VERA_ADDR_HIGH
    ldy #4
    ldx #0
clear:
    stz VERA_DATA0
    inx
    bne clear
    dey
    bne clear
    lda #128
    sta $9f2a
    sta $9f2b
    ; Sprite clearing wraps the VRAM pointer; select bank 1 again.
    ; Expand the built-in loading label to 4bpp, with transparent padding.
    stz VERA_ADDR_LOW
    lda #$f0
    sta VERA_ADDR_HIGH
    lda #$11
    sta VERA_ADDR_BANK
    ldy #2
    ldx #0
clear_label:
    stz VERA_DATA0
    inx
    bne clear_label
    dey
    bne clear_label
    lda #$80
    sta VERA_ADDR_LOW
    lda #$f0
    sta VERA_ADDR_HIGH
    ldx #0
label_byte:
    lda loading_text,x
    sta ZP0
    ldy #4
label_pair:
    lda #0
    asl ZP0
    bcc :+
    lda #$e0
:
    asl ZP0
    bcc :+
    ora #$0e
:
    sta VERA_DATA0
    dey
    bne label_pair
    inx
    cpx #56
    bne label_byte
    jmp _screen_loading_begin
loading_text:
    .byte $04,$0e,$39,$e7,$d1,$38,$00,$00
    .byte $04,$11,$45,$11,$19,$44,$00,$00
    .byte $04,$11,$45,$11,$19,$40,$00,$00
    .byte $04,$11,$7d,$11,$15,$5c,$00,$00
    .byte $04,$11,$45,$11,$13,$44,$00,$00
    .byte $04,$11,$45,$11,$13,$44,$00,$00
    .byte $07,$ce,$45,$e7,$d1,$38,$41,$04
.endproc

; Text-only scene loader; no disk assets or scene layers are needed.
.proc _screen_loading_begin
    stz VERA_ADDR_CTRL
    stz VERA_ADDR_LOW
    lda #$fc
    sta VERA_ADDR_HIGH
    lda #$11
    sta VERA_ADDR_BANK
    ldy #3
    ldx #0
clear:
    stz VERA_DATA0
    inx
    bne clear
    dey
    bne clear
    ; Sprite 80: 64x16 at (288,232), with the selected theme palette.
    lda #$80
    sta VERA_ADDR_LOW
    lda #$fe
    sta VERA_ADDR_HIGH
    ldx #0
label:
    lda attributes,x
    cpx #7
    bne :+
    lda _screen_style
    cmp #4
    bne palette_bank
    lda #$74              ; Matrix label uses palette index 78
    bra :+
palette_bank:
    ora #$72
:
    sta VERA_DATA0
    inx
    cpx #8
    bne label
    stz VERA_ADDR_LOW
    lda #$fa
    sta VERA_ADDR_HIGH
    lda _screen_style
    beq background
    cmp #2
    beq foundry
    cmp #4
    bne :+
    lda #$11               ; Matrix charcoal green
    bra background
:
    lda #$12               ; Cyberpunk navy
    bra background
foundry:
    lda #$22               ; Foundry steel, from its sixteen-color palette
background:
    sta VERA_DATA0
    lda #0
    ldx _screen_style
    cpx #2
    bne :+
    lda #2
:
    cpx #3
    bne :+
    lda #1               ; Synthwave deep plum ($112)
:
    sta VERA_DATA0
    stz $9f2c              ; black border
    lda #$41
    sta $9f29
    rts
attributes:
    .byte $80,$0f,$20,$01,$e8,$00,$0c,$72
.endproc

_screen_transition_begin = _screen_loading_begin

; fastcall A = finished scene display flags; show it as soon as it is ready.
.proc _screen_transition_end
    pha
    and #$30
    bne ready
    pla
    jmp _screen_loading_begin ; palette updates retain the themed loader
ready:
    pla
    sta $9f29
    rts
.endproc
