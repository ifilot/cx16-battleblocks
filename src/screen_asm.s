.include "vera.inc"
.include "constants.inc"
.include "zeropage.inc"

.import popa

.export _screen_fill_layer

;------------------------------------------------------------------------------
; ROUTINE _screen_fill_layer
;
; void screen_fill_layer(uint8_t layer, uint8_t tile_id) is called using a 
; __fastcall__ directive, which means that `tile_id` is placed in A while 
; `layer` is placed on the C-stack.
;
; C-stack
;            +------------------+
;            | layer            |
; Offset 0 ->+------------------+
;
; A single `popa` command will decrement sp and put `layer` into A
;------------------------------------------------------------------------------
_screen_fill_layer:
    sta ZP1                 ; store tile id on ZP1
    jsr popa
    sta ZP0
    stz VERA_ADDR_LOW       ; set VRAM low byte (always $00)
    ldx ZP0                 ; load offset
    lda mapbase_mb,x        ; load high byte
    sta VERA_ADDR_HIGH      ; set VRAM high byte
    lda #$11                ; load VRAM bank
    sta VERA_ADDR_BANK      ; store VRAM bank (assume incr 1)
    ldx #64                 ; load number of rows
@nextrow:
    lda ZP1                 ; load tile
    ldy #64                 ; load number of columns
@nextcol:
    sta VERA_DATA0          ; store tile id
    stz VERA_DATA0          ; store tile properties
    dey
    bne @nextcol
    dex
    bne @nextrow
    rts

; layer address
mapbase_mb:
    .byte $80, $A0