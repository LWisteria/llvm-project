; RUN: llc < %s -mtriple=ve | FileCheck %s

; Function Attrs: norecurse nounwind readnone
define zeroext i1 @func_float_1(float %0, float %1, i1 zeroext %2, i1 zeroext %3) {
; CHECK-LABEL: func_float_1:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i1 %2, i1 %3
  ret i1 %6
}

; Function Attrs: norecurse nounwind readnone
define signext i8 @func_float_8(float %0, float %1, i8 signext %2, i8 signext %3) {
; CHECK-LABEL: func_float_8:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i8 %2, i8 %3
  ret i8 %6
}

; Function Attrs: norecurse nounwind readnone
define zeroext i8 @func_float_u8(float %0, float %1, i8 zeroext %2, i8 zeroext %3) {
; CHECK-LABEL: func_float_u8:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i8 %2, i8 %3
  ret i8 %6
}

; Function Attrs: norecurse nounwind readnone
define signext i16 @func_float_16(float %0, float %1, i16 signext %2, i16 signext %3) {
; CHECK-LABEL: func_float_16:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i16 %2, i16 %3
  ret i16 %6
}

; Function Attrs: norecurse nounwind readnone
define zeroext i16 @func_float_u16(float %0, float %1, i16 zeroext %2, i16 zeroext %3) {
; CHECK-LABEL: func_float_u16:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i16 %2, i16 %3
  ret i16 %6
}

; Function Attrs: norecurse nounwind readnone
define signext i32 @func_float_32(float %0, float %1, i32 signext %2, i32 signext %3) {
; CHECK-LABEL: func_float_32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i32 %2, i32 %3
  ret i32 %6
}

; Function Attrs: norecurse nounwind readnone
define zeroext i32 @func_float_u32(float %0, float %1, i32 zeroext %2, i32 zeroext %3) {
; CHECK-LABEL: func_float_u32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s3, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i32 %2, i32 %3
  ret i32 %6
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_64(float %0, float %1, i64 %2, i64 %3) {
; CHECK-LABEL: func_float_64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i64 %2, i64 %3
  ret i64 %6
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_u64(float %0, float %1, i64 %2, i64 %3) {
; CHECK-LABEL: func_float_u64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i64 %2, i64 %3
  ret i64 %6
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_128(float %0, float %1, i128 %2, i128 %3) {
; CHECK-LABEL: func_float_128:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i128 %2, i128 %3
  ret i128 %6
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_u128(float %0, float %1, i128 %2, i128 %3) {
; CHECK-LABEL: func_float_u128:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, i128 %2, i128 %3
  ret i128 %6
}

; Function Attrs: norecurse nounwind readnone
define float @func_float_float(float %0, float %1, float %2, float %3) {
; CHECK-LABEL: func_float_float:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, float %2, float %3
  ret float %6
}

; Function Attrs: norecurse nounwind readnone
define double @func_float_double(float %0, float %1, double %2, double %3) {
; CHECK-LABEL: func_float_double:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s3, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, double %2, double %3
  ret double %6
}

; Function Attrs: norecurse nounwind readnone
define fp128 @func_float_quad(float %0, float %1, fp128 %2, fp128 %3) {
; CHECK-LABEL: func_float_quad:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %5 = fcmp oeq float %0, %1
  %6 = select i1 %5, fp128 %2, fp128 %3
  ret fp128 %6
}

; Function Attrs: norecurse nounwind readnone
define { float, float } @func_float_fcomp(float %0, float %1, float %2, float %3, float %4, float %5) {
; CHECK-LABEL: func_float_fcomp:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %7 = fcmp oeq float %0, %1
  %8 = select i1 %7, float %2, float %4
  %9 = select i1 %7, float %3, float %5
  %10 = insertvalue { float, float } undef, float %8, 0
  %11 = insertvalue { float, float } %10, float %9, 1
  ret { float, float } %11
}

; Function Attrs: norecurse nounwind readnone
define { double, double } @func_float_dcomp(float %0, float %1, double %2, double %3, double %4, double %5) {
; CHECK-LABEL: func_float_dcomp:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %7 = fcmp oeq float %0, %1
  %8 = select i1 %7, double %2, double %4
  %9 = select i1 %7, double %3, double %5
  %10 = insertvalue { double, double } undef, double %8, 0
  %11 = insertvalue { double, double } %10, double %9, 1
  ret { double, double } %11
}

; Function Attrs: norecurse nounwind readnone
define { fp128, fp128 } @func_float_qcomp(float %0, float %1, fp128 %2, fp128 %3, fp128 %4, fp128 %5) {
; CHECK-LABEL: func_float_qcomp:
; CHECK:       # %bb.0:
; CHECK-NEXT:    ld %s35, 240(, %s11)
; CHECK-NEXT:    ld %s34, 248(, %s11)
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s6, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s7, %s3, %s0
; CHECK-NEXT:    cmov.s.eq %s34, %s4, %s0
; CHECK-NEXT:    cmov.s.eq %s35, %s5, %s0
; CHECK-NEXT:    or %s0, 0, %s6
; CHECK-NEXT:    or %s1, 0, %s7
; CHECK-NEXT:    or %s2, 0, %s34
; CHECK-NEXT:    or %s3, 0, %s35
; CHECK-NEXT:    b.l.t (, %s10)
  %7 = fcmp oeq float %0, %1
  %8 = select i1 %7, fp128 %2, fp128 %4
  %9 = select i1 %7, fp128 %3, fp128 %5
  %10 = insertvalue { fp128, fp128 } undef, fp128 %8, 0
  %11 = insertvalue { fp128, fp128 } %10, fp128 %9, 1
  ret { fp128, fp128 } %11
}

; Function Attrs: norecurse nounwind readnone
define zeroext i1 @func_float_1_zero(float %0, i1 zeroext %1, i1 zeroext %2) {
; CHECK-LABEL: func_float_1_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i1 %1, i1 %2
  ret i1 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i8 @func_float_8_zero(float %0, i8 signext %1, i8 signext %2) {
; CHECK-LABEL: func_float_8_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i8 @func_float_u8_zero(float %0, i8 zeroext %1, i8 zeroext %2) {
; CHECK-LABEL: func_float_u8_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i16 @func_float_16_zero(float %0, i16 signext %1, i16 signext %2) {
; CHECK-LABEL: func_float_16_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i16 @func_float_u16_zero(float %0, i16 zeroext %1, i16 zeroext %2) {
; CHECK-LABEL: func_float_u16_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i32 @func_float_32_zero(float %0, i32 signext %1, i32 signext %2) {
; CHECK-LABEL: func_float_32_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i32 @func_float_u32_zero(float %0, i32 zeroext %1, i32 zeroext %2) {
; CHECK-LABEL: func_float_u32_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_64_zero(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_64_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_u64_zero(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_u64_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_128_zero(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_128_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_u128_zero(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_u128_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define float @func_float_float_zero(float %0, float %1, float %2) {
; CHECK-LABEL: func_float_float_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, float %1, float %2
  ret float %5
}

; Function Attrs: norecurse nounwind readnone
define double @func_float_double_zero(float %0, double %1, double %2) {
; CHECK-LABEL: func_float_double_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, double %1, double %2
  ret double %5
}

; Function Attrs: norecurse nounwind readnone
define fp128 @func_float_quad_zero(float %0, fp128 %1, fp128 %2) {
; CHECK-LABEL: func_float_quad_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 0.000000e+00
  %5 = select i1 %4, fp128 %1, fp128 %2
  ret fp128 %5
}

; Function Attrs: norecurse nounwind readnone
define { float, float } @func_float_fcomp_zero(float %0, float %1, float %2, float %3, float %4) {
; CHECK-LABEL: func_float_fcomp_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 0.000000e+00
  %7 = select i1 %6, float %1, float %3
  %8 = select i1 %6, float %2, float %4
  %9 = insertvalue { float, float } undef, float %7, 0
  %10 = insertvalue { float, float } %9, float %8, 1
  ret { float, float } %10
}

; Function Attrs: norecurse nounwind readnone
define { double, double } @func_float_dcomp_zero(float %0, double %1, double %2, double %3, double %4) {
; CHECK-LABEL: func_float_dcomp_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 0.000000e+00
  %7 = select i1 %6, double %1, double %3
  %8 = select i1 %6, double %2, double %4
  %9 = insertvalue { double, double } undef, double %7, 0
  %10 = insertvalue { double, double } %9, double %8, 1
  ret { double, double } %10
}

; Function Attrs: norecurse nounwind readnone
define { fp128, fp128 } @func_float_qcomp_zero(float %0, fp128 %1, fp128 %2, fp128 %3, fp128 %4) {
; CHECK-LABEL: func_float_qcomp_zero:
; CHECK:       # %bb.0:
; CHECK-NEXT:    ld %s35, 240(, %s11)
; CHECK-NEXT:    ld %s34, 248(, %s11)
; CHECK-NEXT:    cmov.s.eq %s6, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s7, %s3, %s0
; CHECK-NEXT:    cmov.s.eq %s34, %s4, %s0
; CHECK-NEXT:    cmov.s.eq %s35, %s5, %s0
; CHECK-NEXT:    or %s0, 0, %s6
; CHECK-NEXT:    or %s1, 0, %s7
; CHECK-NEXT:    or %s2, 0, %s34
; CHECK-NEXT:    or %s3, 0, %s35
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 0.000000e+00
  %7 = select i1 %6, fp128 %1, fp128 %3
  %8 = select i1 %6, fp128 %2, fp128 %4
  %9 = insertvalue { fp128, fp128 } undef, fp128 %7, 0
  %10 = insertvalue { fp128, fp128 } %9, fp128 %8, 1
  ret { fp128, fp128 } %10
}

; Function Attrs: norecurse nounwind readnone
define zeroext i1 @func_float_1_i(float %0, i1 zeroext %1, i1 zeroext %2) {
; CHECK-LABEL: func_float_1_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i1 %1, i1 %2
  ret i1 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i8 @func_float_8_i(float %0, i8 signext %1, i8 signext %2) {
; CHECK-LABEL: func_float_8_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i8 @func_float_u8_i(float %0, i8 zeroext %1, i8 zeroext %2) {
; CHECK-LABEL: func_float_u8_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i16 @func_float_16_i(float %0, i16 signext %1, i16 signext %2) {
; CHECK-LABEL: func_float_16_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i16 @func_float_u16_i(float %0, i16 zeroext %1, i16 zeroext %2) {
; CHECK-LABEL: func_float_u16_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i32 @func_float_32_i(float %0, i32 signext %1, i32 signext %2) {
; CHECK-LABEL: func_float_32_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i32 @func_float_u32_i(float %0, i32 zeroext %1, i32 zeroext %2) {
; CHECK-LABEL: func_float_u32_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_64_i(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_64_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_u64_i(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_u64_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_128_i(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_128_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_u128_i(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_u128_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define float @func_float_float_i(float %0, float %1, float %2) {
; CHECK-LABEL: func_float_float_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, float %1, float %2
  ret float %5
}

; Function Attrs: norecurse nounwind readnone
define double @func_float_double_i(float %0, double %1, double %2) {
; CHECK-LABEL: func_float_double_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, double %1, double %2
  ret double %5
}

; Function Attrs: norecurse nounwind readnone
define fp128 @func_float_quad_i(float %0, fp128 %1, fp128 %2) {
; CHECK-LABEL: func_float_quad_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s1, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, 1.200000e+01
  %5 = select i1 %4, fp128 %1, fp128 %2
  ret fp128 %5
}

; Function Attrs: norecurse nounwind readnone
define { float, float } @func_float_fcomp_i(float %0, float %1, float %2, float %3, float %4) {
; CHECK-LABEL: func_float_fcomp_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 1.200000e+01
  %7 = select i1 %6, float %1, float %3
  %8 = select i1 %6, float %2, float %4
  %9 = insertvalue { float, float } undef, float %7, 0
  %10 = insertvalue { float, float } %9, float %8, 1
  ret { float, float } %10
}

; Function Attrs: norecurse nounwind readnone
define { double, double } @func_float_dcomp_i(float %0, double %1, double %2, double %3, double %4) {
; CHECK-LABEL: func_float_dcomp_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 1.200000e+01
  %7 = select i1 %6, double %1, double %3
  %8 = select i1 %6, double %2, double %4
  %9 = insertvalue { double, double } undef, double %7, 0
  %10 = insertvalue { double, double } %9, double %8, 1
  ret { double, double } %10
}

; Function Attrs: norecurse nounwind readnone
define { fp128, fp128 } @func_float_qcomp_i(float %0, fp128 %1, fp128 %2, fp128 %3, fp128 %4) {
; CHECK-LABEL: func_float_qcomp_i:
; CHECK:       # %bb.0:
; CHECK-NEXT:    ld %s35, 240(, %s11)
; CHECK-NEXT:    ld %s34, 248(, %s11)
; CHECK-NEXT:    lea.sl %s1, 1094713344
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s6, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s7, %s3, %s0
; CHECK-NEXT:    cmov.s.eq %s34, %s4, %s0
; CHECK-NEXT:    cmov.s.eq %s35, %s5, %s0
; CHECK-NEXT:    or %s0, 0, %s6
; CHECK-NEXT:    or %s1, 0, %s7
; CHECK-NEXT:    or %s2, 0, %s34
; CHECK-NEXT:    or %s3, 0, %s35
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, 1.200000e+01
  %7 = select i1 %6, fp128 %1, fp128 %3
  %8 = select i1 %6, fp128 %2, fp128 %4
  %9 = insertvalue { fp128, fp128 } undef, fp128 %7, 0
  %10 = insertvalue { fp128, fp128 } %9, fp128 %8, 1
  ret { fp128, fp128 } %10
}

; Function Attrs: norecurse nounwind readnone
define zeroext i1 @func_float_1_m(float %0, i1 zeroext %1, i1 zeroext %2) {
; CHECK-LABEL: func_float_1_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i1 %1, i1 %2
  ret i1 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i8 @func_float_8_m(float %0, i8 signext %1, i8 signext %2) {
; CHECK-LABEL: func_float_8_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i8 @func_float_u8_m(float %0, i8 zeroext %1, i8 zeroext %2) {
; CHECK-LABEL: func_float_u8_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i8 %1, i8 %2
  ret i8 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i16 @func_float_16_m(float %0, i16 signext %1, i16 signext %2) {
; CHECK-LABEL: func_float_16_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i16 @func_float_u16_m(float %0, i16 zeroext %1, i16 zeroext %2) {
; CHECK-LABEL: func_float_u16_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i16 %1, i16 %2
  ret i16 %5
}

; Function Attrs: norecurse nounwind readnone
define signext i32 @func_float_32_m(float %0, i32 signext %1, i32 signext %2) {
; CHECK-LABEL: func_float_32_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.sx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define zeroext i32 @func_float_u32_m(float %0, i32 zeroext %1, i32 zeroext %2) {
; CHECK-LABEL: func_float_u32_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    adds.w.zx %s0, %s2, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i32 %1, i32 %2
  ret i32 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_64_m(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_64_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i64 @func_float_u64_m(float %0, i64 %1, i64 %2) {
; CHECK-LABEL: func_float_u64_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i64 %1, i64 %2
  ret i64 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_128_m(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_128_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define i128 @func_float_u128_m(float %0, i128 %1, i128 %2) {
; CHECK-LABEL: func_float_u128_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, i128 %1, i128 %2
  ret i128 %5
}

; Function Attrs: norecurse nounwind readnone
define float @func_float_float_m(float %0, float %1, float %2) {
; CHECK-LABEL: func_float_float_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, float %1, float %2
  ret float %5
}

; Function Attrs: norecurse nounwind readnone
define double @func_float_double_m(float %0, double %1, double %2) {
; CHECK-LABEL: func_float_double_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s3, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s3
; CHECK-NEXT:    cmov.s.eq %s2, %s1, %s0
; CHECK-NEXT:    or %s0, 0, %s2
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, double %1, double %2
  ret double %5
}

; Function Attrs: norecurse nounwind readnone
define fp128 @func_float_quad_m(float %0, fp128 %1, fp128 %2) {
; CHECK-LABEL: func_float_quad_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s1, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s5, %s3, %s0
; CHECK-NEXT:    or %s0, 0, %s4
; CHECK-NEXT:    or %s1, 0, %s5
; CHECK-NEXT:    b.l.t (, %s10)
  %4 = fcmp oeq float %0, -2.000000e+00
  %5 = select i1 %4, fp128 %1, fp128 %2
  ret fp128 %5
}

; Function Attrs: norecurse nounwind readnone
define { float, float } @func_float_fcomp_m(float %0, float %1, float %2, float %3, float %4) {
; CHECK-LABEL: func_float_fcomp_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, -2.000000e+00
  %7 = select i1 %6, float %1, float %3
  %8 = select i1 %6, float %2, float %4
  %9 = insertvalue { float, float } undef, float %7, 0
  %10 = insertvalue { float, float } %9, float %8, 1
  ret { float, float } %10
}

; Function Attrs: norecurse nounwind readnone
define { double, double } @func_float_dcomp_m(float %0, double %1, double %2, double %3, double %4) {
; CHECK-LABEL: func_float_dcomp_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s5, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s5
; CHECK-NEXT:    cmov.s.eq %s3, %s1, %s0
; CHECK-NEXT:    cmov.s.eq %s4, %s2, %s0
; CHECK-NEXT:    or %s0, 0, %s3
; CHECK-NEXT:    or %s1, 0, %s4
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, -2.000000e+00
  %7 = select i1 %6, double %1, double %3
  %8 = select i1 %6, double %2, double %4
  %9 = insertvalue { double, double } undef, double %7, 0
  %10 = insertvalue { double, double } %9, double %8, 1
  ret { double, double } %10
}

; Function Attrs: norecurse nounwind readnone
define { fp128, fp128 } @func_float_qcomp_m(float %0, fp128 %1, fp128 %2, fp128 %3, fp128 %4) {
; CHECK-LABEL: func_float_qcomp_m:
; CHECK:       # %bb.0:
; CHECK-NEXT:    ld %s35, 240(, %s11)
; CHECK-NEXT:    ld %s34, 248(, %s11)
; CHECK-NEXT:    lea.sl %s1, -1073741824
; CHECK-NEXT:    fcmp.s %s0, %s0, %s1
; CHECK-NEXT:    cmov.s.eq %s6, %s2, %s0
; CHECK-NEXT:    cmov.s.eq %s7, %s3, %s0
; CHECK-NEXT:    cmov.s.eq %s34, %s4, %s0
; CHECK-NEXT:    cmov.s.eq %s35, %s5, %s0
; CHECK-NEXT:    or %s0, 0, %s6
; CHECK-NEXT:    or %s1, 0, %s7
; CHECK-NEXT:    or %s2, 0, %s34
; CHECK-NEXT:    or %s3, 0, %s35
; CHECK-NEXT:    b.l.t (, %s10)
  %6 = fcmp oeq float %0, -2.000000e+00
  %7 = select i1 %6, fp128 %1, fp128 %3
  %8 = select i1 %6, fp128 %2, fp128 %4
  %9 = insertvalue { fp128, fp128 } undef, fp128 %7, 0
  %10 = insertvalue { fp128, fp128 } %9, fp128 %8, 1
  ret { fp128, fp128 } %10
}
