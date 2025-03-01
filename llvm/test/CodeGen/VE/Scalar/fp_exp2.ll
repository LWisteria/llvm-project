; RUN: llc < %s -mtriple=ve | FileCheck %s

; Function Attrs: nofree nounwind
define float @func_fp_exp2f_var_float(float %0) {
; CHECK-LABEL: func_fp_exp2f_var_float:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s1, exp2f@lo
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    lea.sl %s12, exp2f@hi(, %s1)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call float @exp2f(float %0)
  ret float %2
}

; Function Attrs: nofree nounwind
declare float @exp2f(float)

; Function Attrs: nofree nounwind
define double @func_fp_exp2_var_double(double %0) {
; CHECK-LABEL: func_fp_exp2_var_double:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s1, exp2@lo
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    lea.sl %s12, exp2@hi(, %s1)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call double @exp2(double %0)
  ret double %2
}

; Function Attrs: nofree nounwind
declare double @exp2(double)

; Function Attrs: nofree nounwind
define fp128 @func_fp_exp2l_var_quad(fp128 %0) {
; CHECK-LABEL: func_fp_exp2l_var_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s2, exp2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, exp2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call fp128 @exp2l(fp128 %0)
  ret fp128 %2
}

; Function Attrs: nofree nounwind
declare fp128 @exp2l(fp128)

; Function Attrs: norecurse nounwind readnone
define float @func_fp_exp2f_zero_float() {
; CHECK-LABEL: func_fp_exp2f_zero_float:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s0, 1065353216
; CHECK-NEXT:    b.l.t (, %s10)
  ret float 1.000000e+00
}

; Function Attrs: norecurse nounwind readnone
define double @func_fp_EXP2_zero_double() {
; CHECK-LABEL: func_fp_EXP2_zero_double:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s0, 1072693248
; CHECK-NEXT:    b.l.t (, %s10)
  ret double 1.000000e+00
}

; Function Attrs: nofree nounwind
define fp128 @func_fp_exp2l_zero_quad() {
; CHECK-LABEL: func_fp_exp2l_zero_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s0)
; CHECK-NEXT:    ld %s0, 8(, %s2)
; CHECK-NEXT:    ld %s1, (, %s2)
; CHECK-NEXT:    lea %s2, exp2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, exp2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call fp128 @exp2l(fp128 0xL00000000000000000000000000000000)
  ret fp128 %1
}

; Function Attrs: norecurse nounwind readnone
define float @func_fp_exp2f_const_float() {
; CHECK-LABEL: func_fp_exp2f_const_float:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s0, 1048576000
; CHECK-NEXT:    b.l.t (, %s10)
  ret float 2.500000e-01
}

; Function Attrs: norecurse nounwind readnone
define double @func_fp_exp2_const_double() {
; CHECK-LABEL: func_fp_exp2_const_double:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s0, 1070596096
; CHECK-NEXT:    b.l.t (, %s10)
  ret double 2.500000e-01
}

; Function Attrs: nofree nounwind
define fp128 @func_fp_exp2l_const_quad() {
; CHECK-LABEL: func_fp_exp2l_const_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s0)
; CHECK-NEXT:    ld %s0, 8(, %s2)
; CHECK-NEXT:    ld %s1, (, %s2)
; CHECK-NEXT:    lea %s2, exp2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, exp2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call fp128 @exp2l(fp128 0xL0000000000000000C000000000000000)
  ret fp128 %1
}
