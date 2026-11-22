; Pops from an empty stack on purpose: the CPU must stop with a StackUnderflow fault.
; (A write into ROM would be caught earlier: the assembler already refuses it.)
start:
    LDI  R0, 42
    POP  R1             ; nothing was pushed
    HLT
