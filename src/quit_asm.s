.export _screen_modal_copy
.importzp ptr1

.bss
restore_mode: .res 1

.code
; Save/restore both 8 KB tilemaps using scratch RAM banks 3 and 4.
; Audio ISR preserves the selected RAM bank and VERA state.
.proc _screen_modal_copy
    sta restore_mode
    lda $00
    pha
    lda #3
    sta $00
    stz ptr1
    lda #$a0
    sta ptr1+1
    stz $9f25
    stz $9f20
    lda #$80
    sta $9f21
    lda #$11
    sta $9f22
    ldx #64
    ldy #0
loop:
    lda restore_mode
    bne restore
    lda $9f23
    sta (ptr1),y
    bra next
restore:
    lda (ptr1),y
    sta $9f23
next:
    iny
    bne loop
    inc ptr1+1
    lda ptr1+1
    cmp #$c0
    bne page
    lda #$a0
    sta ptr1+1
    inc $00
page:
    dex
    bne loop
    pla
    sta $00
    rts
.endproc
