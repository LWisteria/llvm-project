# RUN: rm -rf %t && mkdir -p %t
# RUN: llvm-mc -triple=ve -filetype=obj -o %t/module.o %p/Inputs/module.s
# RUN: llvm-mc -triple=ve -filetype=obj -o %t/symbols.o %s
# RUN: llvm-rtdyld -triple=ve -verify -check=%s %t/symbols.o %t/module.o

var_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_chk, 3) = var[31:0]
        lea %s0, var
var_lo_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_lo_chk, 3) = var[31:0]
        lea %s1, var@lo
        and %s1, %s1, (32)0
var_hi_chk:
# Check the higher 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_hi_chk, 3) = var[63:32]
        lea.sl %s1, var@hi(, %s1)
var_lo8_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_lo8_chk, 3) = (var + 8)[31:0]
        lea %s1, var+8@lo
        and %s1, %s1, (32)0
var_hi8_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_hi8_chk, 3) = (var + 8)[63:32]
        lea.sl %s1, var+8@hi(, %s1)
