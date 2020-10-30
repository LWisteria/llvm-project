	.text
	.file	"ELF_x86_64_StubBuf.ll"
	.globl	f                               # -- Begin function f
	.p2align	4
	.type	f,@function
f:                                      # @f
# %bb.0:                                # %entry
	st %s9, (, %s11)
	st %s10, 8(, %s11)
	st %s15, 24(, %s11)
	st %s16, 32(, %s11)
	or %s9, 0, %s11
	lea %s13, -240
	and %s13, %s13, (32)0
	lea.sl %s11, -1(%s13, %s11)
	brge.l.t %s11, %s8, .LBB0_2
# %bb.1:                                # %entry
	ld %s61, 24(, %s14)
	or %s62, 0, %s0
	lea %s63, 315
	shm.l %s63, (%s61)
	shm.l %s8, 8(%s61)
	shm.l %s11, 16(%s61)
	monc
	or %s0, 0, %s62
.LBB0_2:                                # %entry
	st %s18, 48(, %s9)                      # 8-byte Folded Spill
	lea %s0, g@lo
	and %s0, %s0, (32)0
	lea.sl %s18, g@hi(, %s0)
	or %s12, 0, %s18
	bsic %s10, (, %s12)
	or %s12, 0, %s18
	bsic %s10, (, %s12)
	or %s12, 0, %s18
	bsic %s10, (, %s12)
	ld %s18, 48(, %s9)                      # 8-byte Folded Reload
	or %s11, 0, %s9
	ld %s16, 32(, %s11)
	ld %s15, 24(, %s11)
	ld %s10, 8(, %s11)
	ld %s9, (, %s11)
	b.l.t (, %s10)
.Lfunc_end0:
	.size	f, .Lfunc_end0-f
                                        # -- End function
	.section	".note.GNU-stack","",@progbits
