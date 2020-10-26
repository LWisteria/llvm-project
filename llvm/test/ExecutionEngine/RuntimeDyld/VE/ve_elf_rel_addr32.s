# RUN: llvm-mc -triple=ve -filetype=obj -o %t %s
# RUN: llvm-rtdyld -triple=ve -verify -check=%s %t
	.text
	.file	"ve_elf_rel_addr32.c"
	.globl	check_str                       # -- Begin function check_str
	.p2align	4
	.type	check_str,@function
check_str:                              # @check_str
# %bb.0:                                # %entry
insn_lo:
# Check the lower 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(insn_lo, 3) = elements[31:0]
	lea %s0, elements@lo
	and %s0, %s0, (32)0
insn_hi:
# Check the higher 32-bits of the symbol's absolute address
# rtdyld-check: decode_operand(insn_hi, 3) = elements[63:32]
	lea.sl %s0, elements@hi(, %s0)
	b.l.t (, %s10)
.Lfunc_end0:
	.size	check_str, .Lfunc_end0-check_str
                                        # -- End function
	.type	elements,@object                  # @.str
	.section	.rodata.str1.1,"aMS",@progbits,1
elements:
	.asciz	"hello"
	.size	elements, 6

	.ident	"clang version 12.0.0 (git@socsv218.svp.cl.nec.co.jp:ve-llvm/llvm-project.git 8225a5a97cdc33e7d890510f21b1dd877ddefdb1)"
	.section	".note.GNU-stack","",@progbits
