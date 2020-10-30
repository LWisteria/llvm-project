# RUN: llvm-mc -triple=ve -filetype=obj -o %t %s
# RUN: llvm-rtdyld -triple=ve -verify -check=%s %t

var:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var, 3) = var[31:0]
        lea %s0, var
var_lo:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_lo, 3) = var[31:0]
        lea %s1, var@lo
        and %s1, %s1, (32)0
var_hi:
# Check the higher 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_hi, 3) = var[63:32]
        lea.sl %s1, var@hi(, %s1)
var_lo8:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_lo8, 3) = (var + 8)[31:0]
        lea %s1, var+8@lo
        and %s1, %s1, (32)0
var_hi8:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(var_hi8, 3) = (var + 8)[63:32]
        lea.sl %s1, var+8@hi(, %s1)
