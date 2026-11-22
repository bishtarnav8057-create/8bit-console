; Prints HELLO PIXEL-8 through the text port 0xF004, lights the screen corners, then halts.
start:
    LDI  R0, 72         ; H
    ST   R0, [0xF004]
    LDI  R0, 69         ; E
    ST   R0, [0xF004]
    LDI  R0, 76         ; L
    ST   R0, [0xF004]
    ST   R0, [0xF004]
    LDI  R0, 79         ; O
    ST   R0, [0xF004]
    LDI  R0, 32         ; space
    ST   R0, [0xF004]
    LDI  R0, 80         ; P
    ST   R0, [0xF004]
    LDI  R0, 73         ; I
    ST   R0, [0xF004]
    LDI  R0, 88         ; X
    ST   R0, [0xF004]
    LDI  R0, 69         ; E
    ST   R0, [0xF004]
    LDI  R0, 76         ; L
    ST   R0, [0xF004]
    LDI  R0, 45         ; -
    ST   R0, [0xF004]
    LDI  R0, 56         ; 8
    ST   R0, [0xF004]
    LDI  R0, 2          ; amber
    ST   R0, [0xE000]   ; top-left pixel
    LDI  R0, 3          ; red
    ST   R0, [0xE01F]   ; top-right
    LDI  R0, 4          ; cyan
    ST   R0, [0xE1E0]   ; bottom-left (y 15: 0xE000 + 15*32)
    LDI  R0, 1          ; green
    ST   R0, [0xE1FF]   ; bottom-right
    HLT
