# RUN: rm -rf %t && mkdir -p %t
# RUN: llvm-mc -triple=ve --position-independent -filetype=obj -o %t/module.o %p/Inputs/module.s
# RUN: llvm-mc -triple=ve --position-independent -filetype=obj -o %t/symbols.o %s
# RUN: llvm-rtdyld -triple=ve -verify -check=%s %t/symbols.o %t/module.o

got_lo_chk:
# rtdyld-check: decode_operand(got_lo_chk, 1) = section_addr(t.o, .got) - lea_static_data
    lea %s15, _GLOBAL_OFFSET_TABLE_@pc_lo(-24)
    and %s15, %s15, (32)0
    sic %s16
got_hi_chk:
    lea.sl %s15, _GLOBAL_OFFSET_TABLE_@pc_hi(%s16, %s15)
var_gotlo_chk:
# rtdyld-check: decode_operand(var_gotlo_chk, 1) = static_data - section_addr(t.o, .got)
    lea %s0, var@got_lo
    and %s0, %s0, (32)0
var_gothi_chk:
    lea.sl %s0, var@got_hi(, %s0)
    ld %s1, (%s0, %s15)
# CHECK: lea %s15, _GLOBAL_OFFSET_TABLE_@pc_lo(-24)
# CHECK-NEXT: and %s15, %s15, (32)0
# CHECK-NEXT: sic %s16
# CHECK-NEXT: lea.sl %s15, _GLOBAL_OFFSET_TABLE_@pc_hi(%s16, %s15)
# CHECK-NEXT: lea %s0, dst@got_lo
# CHECK-NEXT: and %s0, %s0, (32)0
# CHECK-NEXT: lea.sl %s0, dst@got_hi(, %s0)
# CHECK-NEXT: ld %s1, (%s0, %s15)

# CHECK-OBJ: 0 R_VE_PC_LO32 _GLOBAL_OFFSET_TABLE_
# CHECK-OBJ-NEXT: 18 R_VE_PC_HI32 _GLOBAL_OFFSET_TABLE_
# CHECK-OBJ-NEXT: 20 R_VE_GOT_LO32 dst
# CHECK-OBJ-NEXT: 30 R_VE_GOT_HI32 dst
