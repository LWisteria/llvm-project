### Modules to use for symbolic verifies

    .text
    .globl tgt
tgt:
    b.l.t tgt
    .globl tgt2
tgt2:
    b.l.t tgt

    .type   var,@object                  # @.str
    .data
    .section        .rodata.str1.1,"aMS",@progbits,1
var:
    .asciz  "hello"
    .size   var, 6
