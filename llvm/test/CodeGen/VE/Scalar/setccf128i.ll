; RUN: llc < %s -mtriple=ve-unknown-unknown | FileCheck %s

define zeroext i1 @setccaf(fp128, fp128) {
; CHECK-LABEL: setccaf:
; CHECK:       # %bb.0:
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp false fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccat(fp128, fp128) {
; CHECK-LABEL: setccat:
; CHECK:       # %bb.0:
; CHECK-NEXT:    or %s0, 1, (0)1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp true fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccoeq(fp128, fp128) {
; CHECK-LABEL: setccoeq:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.eq %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp oeq fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccone(fp128, fp128) {
; CHECK-LABEL: setccone:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.ne %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp one fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccogt(fp128, fp128) {
; CHECK-LABEL: setccogt:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.gt %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ogt fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccoge(fp128, fp128) {
; CHECK-LABEL: setccoge:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.ge %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp oge fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccolt(fp128, fp128) {
; CHECK-LABEL: setccolt:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.lt %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp olt fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccole(fp128, fp128) {
; CHECK-LABEL: setccole:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.le %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ole fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccord(fp128, fp128) {
; CHECK-LABEL: setccord:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.q %s1, %s0, %s0
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.num %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ord fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccuno(fp128, fp128) {
; CHECK-LABEL: setccuno:
; CHECK:       # %bb.0:
; CHECK-NEXT:    fcmp.q %s1, %s0, %s0
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.nan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp uno fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccueq(fp128, fp128) {
; CHECK-LABEL: setccueq:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.eqnan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ueq fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccune(fp128, fp128) {
; CHECK-LABEL: setccune:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.nenan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp une fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccugt(fp128, fp128) {
; CHECK-LABEL: setccugt:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.gtnan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ugt fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccuge(fp128, fp128) {
; CHECK-LABEL: setccuge:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.genan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp uge fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccult(fp128, fp128) {
; CHECK-LABEL: setccult:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.ltnan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ult fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}

define zeroext i1 @setccule(fp128, fp128) {
; CHECK-LABEL: setccule:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s2, .LCPI{{[0-9]+}}_0@lo
; CHECK-NEXT:    and %s2, %s2, (32)0
; CHECK-NEXT:    lea.sl %s2, .LCPI{{[0-9]+}}_0@hi(, %s2)
; CHECK-NEXT:    ld %s4, 8(, %s2)
; CHECK-NEXT:    ld %s5, (, %s2)
; CHECK-NEXT:    fcmp.q %s1, %s0, %s4
; CHECK-NEXT:    or %s0, 0, (0)1
; CHECK-NEXT:    cmov.d.lenan %s0, (63)0, %s1
; CHECK-NEXT:    b.l.t (, %s10)
  %3 = fcmp ule fp128 %0, 0xL00000000000000000000000000000000
  ret i1 %3
}
