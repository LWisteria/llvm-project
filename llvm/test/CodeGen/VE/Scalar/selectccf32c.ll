; RUN: llc < %s -mtriple=ve-unknown-unknown | FileCheck %s

define float @selectccsgti8(i8 signext, i8 signext, float, float) {
; CHECK-LABEL: selectccsgti8:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmps.w.sx %s0, %s0, %s1
; CHECK-NEXT:    cmov.w.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = icmp sgt i8 %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccsgti16(i16 signext, i16 signext, float, float) {
; CHECK-LABEL: selectccsgti16:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmps.w.sx %s0, %s0, %s1
; CHECK-NEXT:    cmov.w.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = icmp sgt i16 %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccsgti32(i32 signext, i32 signext, float, float) {
; CHECK-LABEL: selectccsgti32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmps.w.sx %s0, %s0, %s1
; CHECK-NEXT:    cmov.w.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = icmp sgt i32 %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccsgti64(i64, i64, float, float) {
; CHECK-LABEL: selectccsgti64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmps.l %s0, %s0, %s1
; CHECK-NEXT:    cmov.l.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = icmp sgt i64 %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccsgti128(i128, i128, float, float) {
; CHECK-LABEL: selectccsgti128:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmps.l %s6, %s1, %s3
; CHECK-NEXT:    cmps.l %s1, %s3, %s1
; CHECK-NEXT:    srl %s1, %s1, 63
; CHECK-NEXT:    cmpu.l %s0, %s2, %s0
; CHECK-NEXT:    srl %s0, %s0, 63
; CHECK-NEXT:    cmov.l.eq %s1, %s0, %s6
; CHECK-NEXT:    cmov.w.ne %s5, %s4, %s1
; CHECK-NEXT:    or %s0, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = icmp sgt i128 %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccogtf32(float, float, float, float) {
; CHECK-LABEL: selectccogtf32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp ogt float %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

define float @selectccogtf64(double, double, float, float) {
; CHECK-LABEL: selectccogtf64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.d %s0, %s0, %s1
; CHECK-NEXT:    cmov.d.gt %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp ogt double %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

