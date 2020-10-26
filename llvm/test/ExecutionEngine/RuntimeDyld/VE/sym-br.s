# RUN: rm -rf %t && mkdir -p %t
# RUN: llvm-mc -triple=ve -filetype=obj -o %t/module.o %p/Inputs/module.s
# RUN: llvm-mc -triple=ve -filetype=obj -o %t/sym-br.o %s
# RUN: llvm-rtdyld -triple=ve -verify -check=%s %t/sym-br.o %t/module.o

tgt_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(tgt_chk, 1) = tgt[31:0]
        b.l.t tgt
tgt2_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(tgt2_chk, 0) = (tgt2 - tgt2_chk)[31:0]
        br.l.t tgt2
tgt_s1_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(tgt_s1_chk, 1) = tgt[31:0]
        b.l.t tgt(, %s1)
tgt_s1_24_chk:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(tgt_s1_24_chk, 1) = (tgt + 24)[31:0]
        b.l.t tgt+24(, %s1)
