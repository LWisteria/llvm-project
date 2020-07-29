; RUN: llc -O0 --march=ve %s -o=/dev/stdout | FileCheck %s

define void @test_vp_harness(<256 x i64>* %Out, <256 x i64> %i0) {
; CHECK-LABEL: test_vp_harness:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s1, 256
; CHECK-NEXT:    # kill: def $sw1 killed $sw1 killed $sx1
; CHECK-NEXT:    lvl %s1
; CHECK-NEXT:    vst %v0, 8, %s0
; CHECK-NEXT:    or %s11, 0, %s9
  store <256 x i64> %i0, <256 x i64>* %Out
  ret void
}

define void @test_vp_memory_i64(<256 x i64>* %VecPtr, <256 x i64*> %PtrVec, <256 x i64> %i0, <256 x i1> %m, i32 %n) {
; CHECK-LABEL: test_vp_memory_i64:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    # kill: def $sw1 killed $sw1 killed $sx1
; CHECK-NEXT:    lvl %s1
; CHECK-NEXT:    vseq %v1
; CHECK-NEXT:    vmulu.l %v1, 8, %v1, %vm1
; CHECK-NEXT:    vaddu.l %v1, %s0, %v1, %vm1
; CHECK-NEXT:    vgt %v1, %v1, 0, 0, %vm1
; CHECK-NEXT:    vgt %v2, %v0, 0, 0, %vm1
; CHECK-NEXT:    vsc %v0, %v1, 0, 0, %vm1
; CHECK-NEXT:    vst %v2, 8, %s0, %vm1
; CHECK-NEXT:    or %s11, 0, %s9
  %r0 = call <256 x i64> @llvm.vp.load.v256i64.p0v256i64(<256 x i64>* %VecPtr, <256 x i1> %m, i32 %n)
  %r1 = call <256 x i64> @llvm.vp.gather.v256i64.v256p0i64(<256 x i64*> %PtrVec, <256 x i1> %m, i32 %n)
  call void @llvm.vp.scatter.v256i64.v256p0i64(<256 x i64> %r0, <256 x i64*> %PtrVec, <256 x i1> %m, i32 %n)
  call void @llvm.vp.store.v256i64.p0v256i64(<256 x i64> %r1, <256 x i64>* %VecPtr, <256 x i1> %m, i32 %n)
  ret void
}

define void @test_vp_memory_f64(<256 x double>* %VecPtr, <256 x double*> %PtrVec, <256 x double> %i0, <256 x i1> %m, i32 %n) {
; CHECK-LABEL: test_vp_memory_f64:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    # kill: def $sw1 killed $sw1 killed $sx1
; CHECK-NEXT:    lvl %s1
; CHECK-NEXT:    vseq %v1
; CHECK-NEXT:    vmulu.l %v1, 8, %v1, %vm1
; CHECK-NEXT:    vaddu.l %v1, %s0, %v1, %vm1
; CHECK-NEXT:    vgt %v1, %v1, 0, 0, %vm1
; CHECK-NEXT:    vgt %v2, %v0, 0, 0, %vm1
; CHECK-NEXT:    vsc %v0, %v1, 0, 0, %vm1
; CHECK-NEXT:    vst %v2, 8, %s0, %vm1
; CHECK-NEXT:    or %s11, 0, %s9
  %r0 = call <256 x double> @llvm.vp.load.v256f64.p0v256f64(<256 x double>* %VecPtr, <256 x i1> %m, i32 %n)
  %r1 = call <256 x double> @llvm.vp.gather.v256f64.v256p0f64(<256 x double*> %PtrVec, <256 x i1> %m, i32 %n)
  call void @llvm.vp.scatter.v256f64.v256p0f64(<256 x double> %r0, <256 x double*> %PtrVec, <256 x i1> %m, i32 %n)
  call void @llvm.vp.store.v256f64.p0v256f64(<256 x double> %r1, <256 x double>* %VecPtr, <256 x i1> %m, i32 %n)
  ret void
}

; memory - i64
declare void @llvm.vp.store.v256i64.p0v256i64(<256 x i64>, <256 x i64>*, <256 x i1> mask, i32 vlen)
declare <256 x i64> @llvm.vp.load.v256i64.p0v256i64(<256 x i64>*, <256 x i1> mask, i32 vlen)
declare void @llvm.vp.scatter.v256i64.v256p0i64(<256 x i64>, <256 x i64*>, <256 x i1> mask, i32 vlen)
declare <256 x i64> @llvm.vp.gather.v256i64.v256p0i64(<256 x i64*>, <256 x i1> mask, i32 vlen)
; memory - f64
declare void @llvm.vp.store.v256f64.p0v256f64(<256 x double>, <256 x double>*, <256 x i1> mask, i32 vlen)
declare <256 x double> @llvm.vp.load.v256f64.p0v256f64(<256 x double>*, <256 x i1> mask, i32 vlen)
declare void @llvm.vp.scatter.v256f64.v256p0f64(<256 x double>, <256 x double*>, <256 x i1> mask, i32 vlen)
declare <256 x double> @llvm.vp.gather.v256f64.v256p0f64(<256 x double*>, <256 x i1> mask, i32 vlen)
