.export _resources_read_vram

.bss
requested: .res 1
received: .res 1

.code
; fastcall A = 1..255 bytes. Carry selects a fixed destination (VERA DATA0).
.proc _resources_read_vram
    sta requested
    ldx #$23
    ldy #$9F
    sec
    jsr $FF44              ; MACPTR returns actual byte count in X/Y
    bcs fallback
    txa
    ldx #0
    rts
fallback:
    ; Standard KERNAL byte input for devices without MACPTR support.
    stz received
loop:
    jsr $FFCF              ; BASIN
    pha
    jsr $FFB7              ; READST
    and #$BF               ; EOF is valid for the last byte
    bne failed
    pla
    sta $9F23
    inc received
    jsr $FFB7
    bne done
    lda received
    cmp requested
    bne loop
done:
    lda received
    ldx #0
    rts
failed:
    pla
    bra done
.endproc
