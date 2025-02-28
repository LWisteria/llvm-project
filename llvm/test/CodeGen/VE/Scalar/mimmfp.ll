;; Test that a backend correctly handles mimm.

; RUN: llc < %s -mtriple=ve-unknown-unknown | FileCheck %s

define double @mimm_0000000000000000(double %a) {
; CHECK-LABEL: mimm_0000000000000000:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x0000000000000000
  ret double %res
}

define double @mimm_0000000000000001(double %a) {
; CHECK-LABEL: mimm_0000000000000001:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (63)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x0000000000000001
  ret double %res
}

define double @mimm_0000000000000003(double %a) {
; CHECK-LABEL: mimm_0000000000000003:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (62)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x0000000000000003
  ret double %res
}

define double @mimm_000000000000007F(double %a) {
; CHECK-LABEL: mimm_000000000000007F:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (57)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x000000000000007F
  ret double %res
}

define double @mimm_00000000000000FF(double %a) {
; CHECK-LABEL: mimm_00000000000000FF:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (56)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x00000000000000FF
  ret double %res
}

define double @mimm_000000000000FFFF(double %a) {
; CHECK-LABEL: mimm_000000000000FFFF:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (48)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x000000000000FFFF
  ret double %res
}

define double @mimm_000000FFFFFFFFFF(double %a) {
; CHECK-LABEL: mimm_000000FFFFFFFFFF:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (24)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x000000FFFFFFFFFF
  ret double %res
}

define double @mimm_7FFFFFFFFFFFFFFF(double %a) {
; CHECK-LABEL: mimm_7FFFFFFFFFFFFFFF:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (1)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0x7FFFFFFFFFFFFFFF
  ret double %res
}

define double @mimm_FFFFFFFFFFFFFFFF(double %a) {
; CHECK-LABEL: mimm_FFFFFFFFFFFFFFFF:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (0)0
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0xFFFFFFFFFFFFFFFF
  ret double %res
}

define double @mimm_FFFFFFFFFFFFFFFE(double %a) {
; CHECK-LABEL: mimm_FFFFFFFFFFFFFFFE:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (63)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0xFFFFFFFFFFFFFFFE
  ret double %res
}

define double @mimm_FFFFFFFFFFFFFFFC(double %a) {
; CHECK-LABEL: mimm_FFFFFFFFFFFFFFFC:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (62)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0xFFFFFFFFFFFFFFFC
  ret double %res
}

define double @mimm_FFFFFFFFFFFFFF80(double %a) {
; CHECK-LABEL: mimm_FFFFFFFFFFFFFF80:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (57)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0xFFFFFFFFFFFFFF80
  ret double %res
}

define double @mimm_FFFFFFF000000000(double %a) {
; CHECK-LABEL: mimm_FFFFFFF000000000:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (28)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, 0xFFFFFFF000000000
  ret double %res
}

define double @mimm_C000000000000000(double %a) {
; CHECK-LABEL: mimm_C000000000000000:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fadd.d %s0, %s0, (2)1
; CHECK-NEXT:    b.l.t (, %s10)
  %res = fadd double %a, -2.0
  ret double %res
}

define double @mimm_8000000000000000(double %a) {
; CHECK-LABEL: mimm_8000000000000000:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea.sl %s0, -2147483648
; CHECK-NEXT:    b.l.t (, %s10)
  ret double -0.0
}

define float @mimm_007FFFFFFFFFFFFF_float(float %a) {
; CHECK-LABEL: mimm_007FFFFFFFFFFFFF_float:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fmul.s %s0, %s0, (9)0
; CHECK-NEXT:    b.l.t (, %s10)
  %r = fmul float %a, 1.175494210692441075487029444849287348827052428745893333857174530571588870475618904265502351336181163787841796875E-38
  ret float %r
}
