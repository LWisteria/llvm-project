; RUN: llc < %s -mtriple=ve | FileCheck %s

; Function Attrs: nofree nounwind
define float @func_fp_log2f_var_float(float %0) {
; CHECK-LABEL: func_fp_log2f_var_float:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s1, log2f@lo
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    lea.sl %s12, log2f@hi(, %s1)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call float @log2f(float %0)
  ret float %2
}

; Function Attrs: nofree nounwind
declare float @log2f(float)

; Function Attrs: nofree nounwind
define double @func_fp_log2_var_double(double %0) {
; CHECK-LABEL: func_fp_log2_var_double:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s1, log2@lo
; CHECK-NEXT:    and %s1, %s1, (32)0
; CHECK-NEXT:    lea.sl %s12, log2@hi(, %s1)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call double @log2(double %0)
  ret double %2
}

; Function Attrs: nofree nounwind
declare double @log2(double)

; Function Attrs: nofree nounwind
define fp128 @func_fp_log2l_var_quad(fp128 %0) {
; CHECK-LABEL: func_fp_log2l_var_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s2, log2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, log2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %2 = tail call fp128 @log2l(fp128 %0)
  ret fp128 %2
}

; Function Attrs: nofree nounwind
declare fp128 @log2l(fp128)

; Function Attrs: nofree nounwind
define float @func_fp_log2f_zero_float() {
; CHECK-LABEL: func_fp_log2f_zero_float:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, log2f@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s12, log2f@hi(, %s0)
; CHECK-NEXT:    lea.sl %s0, 0
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call float @log2f(float 0.000000e+00)
  ret float %1
}

; Function Attrs: nofree nounwind
define double @func_fp_LOG2_zero_double() {
; CHECK-LABEL: func_fp_LOG2_zero_double:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, log2@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s12, log2@hi(, %s0)
; CHECK-NEXT:    lea.sl %s0, 0
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call double @log2(double 0.000000e+00)
  ret double %1
}

; Function Attrs: nofree nounwind
define fp128 @func_fp_log2l_zero_quad() {
; CHECK-LABEL: func_fp_log2l_zero_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s0)
; CHECK-NEXT:    ld %s0, 8(, %s2)
; CHECK-NEXT:    ld %s1, (, %s2)
; CHECK-NEXT:    lea %s2, log2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, log2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call fp128 @log2l(fp128 0xL00000000000000000000000000000000)
  ret fp128 %1
}

; Function Attrs: nofree nounwind
define float @func_fp_log2f_const_float() {
; CHECK-LABEL: func_fp_log2f_const_float:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, log2f@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s12, log2f@hi(, %s0)
; CHECK-NEXT:    lea.sl %s0, -1073741824
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call float @log2f(float -2.000000e+00)
  ret float %1
}

; Function Attrs: nofree nounwind
define double @func_fp_log2_const_double() {
; CHECK-LABEL: func_fp_log2_const_double:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, log2@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s12, log2@hi(, %s0)
; CHECK-NEXT:    lea.sl %s0, -1073741824
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call double @log2(double -2.000000e+00)
  ret double %1
}

; Function Attrs: nofree nounwind
define fp128 @func_fp_log2l_const_quad() {
; CHECK-LABEL: func_fp_log2l_const_quad:
; CHECK:       .LBB{{[0-9]+}}_2:
; CHECK-NEXT:    lea %s0, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s0, %s0, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s0)
; CHECK-NEXT:    ld %s0, 8(, %s2)
; CHECK-NEXT:    ld %s1, (, %s2)
; CHECK-NEXT:    lea %s2, log2l@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s12, log2l@hi(, %s2)
; CHECK-NEXT:    bsic %s10, (, %s12)
; CHECK-NEXT:    or %s11, 0, %s9
  %1 = tail call fp128 @log2l(fp128 0xL0000000000000000C000000000000000)
  ret fp128 %1
}
