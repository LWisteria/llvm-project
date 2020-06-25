// RUN: %clang_cc1 -triple ve-linux-gnu -emit-llvm %s -o - | FileCheck %s

// CHECK-LABEL: define { float, float } @p(float %a.coerce0, float %a.coerce1, float %b.coerce0, float %b.coerce1) #0 {
float __complex__ p(float __complex__ a, float __complex__ b) {
}

// CHECK-LABEL: define { double, double } @q(double %a.coerce0, double %a.coerce1, double %b.coerce0, double %b.coerce1) #0 {
double __complex__ q(double __complex__ a, double __complex__ b) {
}

// CHECK-LABEL: define { fp128, fp128 } @s(fp128 %a.coerce0, fp128 %a.coerce1, fp128 %b.coerce0, fp128 %b.coerce1) #0 {
long double __complex__ s(long double __complex__ a,
                          long double __complex__ b) {
}

void func() {
  // CHECK-LABEL: %call = call signext i32 (i32, i32, i32, i32, i32, i32, i32, ...) bitcast (i32 (...)* @hoge to i32 (i32, i32, i32, i32, i32, i32, i32, ...)*)(i32 signext 1, i32 signext 2, i32 signext 3, i32 signext 4, i32 signext 5, i32 signext 6, i32 signext 7)
  hoge(1, 2, 3, 4, 5, 6, 7);
}
