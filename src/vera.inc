; -------------------------------------------------------------------
; VERA Hardware Register Definitions for Commander X16
; Suitable for use with ca65
; -------------------------------------------------------------------

; === VERA Control ===
VERA_ADDR_LOW          = $9F20
VERA_ADDR_HIGH         = $9F21
VERA_ADDR_BANK         = $9F22
VERA_DATA0             = $9F23
VERA_DATA1             = $9F24
VERA_ADDR_CTRL         = $9F25

; === Layer 0 Configuration ===
VERA_L0_CONFIG         = $9F2D

; === Layer 1 Configuration ===
VERA_L1_CONFIG         = $9F34