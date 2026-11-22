; A program with mistakes on purpose: the Console must list them by line, not crash.
start:
    LDI  R9, 1          ; there is no R9
    FOO  R0, R1         ; there is no FOO instruction
    LDI  R0, 300        ; 300 does not fit in a byte
    JMP  nowhere        ; this label does not exist
