; PIXEL-8 SUM: the debugger demo program
; Adds 10 + 9 + ... + 1 in a loop and saves the answer (55) in RAM.
;   0x8000 = the answer
; Registers: R0 = total, R1 = counter

start:
    LDI  R0, 0          ; the total starts at 0
    LDI  R1, 10         ; count down from 10
loop:
    ADD  R0, R1         ; add the counter to the total
    DEC  R1             ; one less
    JNZ  loop           ; not at 0 yet: go round again
    ST   R0, [0x8000]   ; save 55 = 10+9+...+1 to RAM
    HLT                 ; stop
