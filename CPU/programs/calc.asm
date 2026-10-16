; PIXEL-8 CALCULATOR
; The C++ side puts two numbers in RAM, starts the CPU at one routine below, and reads the answer back.
;   0x8000 = first number      0x8002 = answer       0x8004 = 1 if a multiply was too big
;   0x8001 = second number     0x8003 = remainder    0x8005 = 1 if asked to divide by zero
; The comment on each line is what the calculator shows next to it.

start:
    HLT                 ; nothing to do until an operation is picked

do_add:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load the second number from RAM
    ADD  R0, R1         ; add them
    ST   R0, [0x8002]   ; save the answer to RAM
    HLT                 ; stop

do_sub:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load the second number from RAM
    SUB  R0, R1         ; subtract the second from the first
    ST   R0, [0x8002]   ; save the answer to RAM
    HLT                 ; stop

do_and:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load the second number from RAM
    AND  R0, R1         ; keep only the bits that are 1 in both
    ST   R0, [0x8002]   ; save the answer to RAM
    HLT                 ; stop

do_or:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load the second number from RAM
    OR   R0, R1         ; keep the bits that are 1 in either
    ST   R0, [0x8002]   ; save the answer to RAM
    HLT                 ; stop

do_xor:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load the second number from RAM
    XOR  R0, R1         ; keep the bits that are different
    ST   R0, [0x8002]   ; save the answer to RAM
    HLT                 ; stop

; The CPU has no multiply instruction, so a * b is a added b times.
do_mul:
    LD   R0, [0x8000]   ; load the first number from RAM
    LD   R1, [0x8001]   ; load how many times to add it
    LDI  R2, 0          ; start the answer at 0
    LDI  R3, 0          ; nothing has gone over 255 yet
    CMPI R1, 0          ; adding it 0 times?
    JZ   mul_done       ; then the answer stays 0
mul_loop:
    ADD  R2, R0         ; add the first number to the answer
    JNC  mul_next       ; no carry: still fits in 8 bits
    LDI  R3, 1          ; carry: the answer got bigger than 255
mul_next:
    DEC  R1             ; one fewer time to go
    JNZ  mul_loop       ; repeat until done
mul_done:
    ST   R2, [0x8002]   ; save the answer to RAM
    ST   R3, [0x8004]   ; save whether it was too big
    HLT                 ; stop

; The CPU has no divide instruction either, so a / b counts how many times b can be taken away.
do_div:
    LD   R0, [0x8000]   ; load the number to divide from RAM
    LD   R1, [0x8001]   ; load what to divide by from RAM
    CMPI R1, 0          ; dividing by zero?
    JZ   div_zero       ; that has no answer
    LDI  R2, 0          ; start the count at 0
div_loop:
    CMP  R0, R1         ; is what is left smaller than the divisor?
    JC   div_done       ; yes: finished
    SUB  R0, R1         ; take the divisor away once more
    INC  R2             ; count it
    JMP  div_loop       ; try again
div_done:
    ST   R2, [0x8002]   ; save the answer to RAM
    ST   R0, [0x8003]   ; save what is left as the remainder
    HLT                 ; stop
div_zero:
    LDI  R3, 1          ; remember it was divide by zero
    ST   R3, [0x8005]   ; save that to RAM
    HLT                 ; stop
