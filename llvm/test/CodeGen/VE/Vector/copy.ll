; RUN: llc < %s -mtriple=ve-unknown-unknown -mattr=+vpu | FileCheck %s

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <256 x i32> @__regcall3__copyv256i32(<256 x i32>, <256 x i32> returned) {
; CHECK-LABEL: __regcall3__copyv256i32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <256 x i32> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <256 x i64> @__regcall3__copyv256i64(<256 x i64>, <256 x i64> returned) {
; CHECK-LABEL: __regcall3__copyv256i64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <256 x i64> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <256 x float> @__regcall3__copyv256f32(<256 x float>, <256 x float> returned) {
; CHECK-LABEL: __regcall3__copyv256f32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <256 x float> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <256 x double> @__regcall3__copyv256f64(<256 x double>, <256 x double> returned) {
; CHECK-LABEL: __regcall3__copyv256f64:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <256 x double> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <512 x i32> @__regcall3__copyv512i32(<512 x i32>, <512 x i32> returned) {
; CHECK-LABEL: __regcall3__copyv512i32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <512 x i32> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <512 x float> @__regcall3__copyv512f32(<512 x float>, <512 x float> returned) {
; CHECK-LABEL: __regcall3__copyv512f32:
; CHECK:       # %bb.0:
; CHECK-NEXT:    lea %s16, 256
; CHECK-NEXT:    lvl %s16
; CHECK-NEXT:    vor %v0, (0)1, %v1
; CHECK-NEXT:    b.l.t (, %s10)
  ret <512 x float> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <256 x i1> @__regcall3__copyv256i1(<256 x i1>, <256 x i1> returned) {
; CHECK-LABEL: __regcall3__copyv256i1:
; CHECK:       # %bb.0:
; CHECK-NEXT:    andm %vm1, %vm0, %vm2
; CHECK-NEXT:    b.l.t (, %s10)
  ret <256 x i1> %1
}

; Function Attrs: norecurse nounwind readnone
define x86_regcallcc <512 x i1> @__regcall3__copyv512i1(<512 x i1>, <512 x i1> returned) {
; CHECK-LABEL: __regcall3__copyv512i1:
; CHECK:       # %bb.0:
; CHECK-NEXT:    andm %vm2, %vm0, %vm4
; CHECK-NEXT:    andm %vm3, %vm0, %vm5
; CHECK-NEXT:    b.l.t (, %s10)
  ret <512 x i1> %1
}

