; PIXEL-8 DOT: the key bridge test program
; A dot on the 32 x 16 screen moves one pixel per frame for each direction button held.
; It stops at the edges. The host sets the buttons, runs one frame, and draws VRAM.
;   0xF000 PORT_INPUT   bit 0 UP, bit 1 DOWN, bit 2 LEFT, bit 3 RIGHT
;   0xF003 PORT_FRAME   goes up by 1 after every frame
;   0x8000 = x, 0x8001 = y   (kept in RAM so the host can show them)
; Registers: R0 = x, R1 = y, R2 = last frame seen, R3 = buttons, R4 = pixel colour, R5 = scratch

start:
    LDI  R0, 16         ; x starts in the middle
    LDI  R1, 8          ; y starts in the middle
    ST   R0, [0x8000]
    ST   R1, [0x8001]
    LD   R2, [0xF003]   ; remember the current frame
    LDI  R4, 1
    CALL plot           ; draw the dot

wait:
    LD   R5, [0xF003]   ; has a new frame started?
    CMP  R5, R2
    JZ   wait           ; no: keep waiting
    MOV  R2, R5
    LD   R3, [0xF000]   ; read the buttons once per frame
    LDI  R4, 0
    CALL plot           ; erase the dot at the old place

    MOV  R5, R3         ; UP: bit 0
    LDI  R4, 1
    AND  R5, R4
    JZ   not_up
    CMPI R1, 0          ; already on the top row?
    JZ   not_up
    DEC  R1
not_up:
    MOV  R5, R3         ; DOWN: bit 1
    LDI  R4, 2
    AND  R5, R4
    JZ   not_down
    CMPI R1, 15         ; already on the bottom row?
    JZ   not_down
    INC  R1
not_down:
    MOV  R5, R3         ; LEFT: bit 2
    LDI  R4, 4
    AND  R5, R4
    JZ   not_left
    CMPI R0, 0          ; already in the left column?
    JZ   not_left
    DEC  R0
not_left:
    MOV  R5, R3         ; RIGHT: bit 3
    LDI  R4, 8
    AND  R5, R4
    JZ   not_right
    CMPI R0, 31         ; already in the right column?
    JZ   not_right
    INC  R0
not_right:
    ST   R0, [0x8000]
    ST   R1, [0x8001]
    LDI  R4, 1
    CALL plot           ; draw the dot at the new place
    JMP  wait

; Writes colour R4 to pixel (R0, R1). Address = 0xE000 + y*32 + x, built in P = R6:R7:
;   R6 (high byte) = 0xE0 + (y >> 3)
;   R7 (low byte)  = ((y & 7) << 5) | x
; Keeps R0 and R1; uses R5.
plot:
    MOV  R6, R1
    SHR  R6
    SHR  R6
    SHR  R6
    ADDI R6, 0xE0
    MOV  R7, R1
    LDI  R5, 7
    AND  R7, R5
    SHL  R7
    SHL  R7
    SHL  R7
    SHL  R7
    SHL  R7
    OR   R7, R0
    STP  R4
    RET
