; RUN: opt -passes='print<loop-dependence>' -disable-output  < %s 2>&1 | FileCheck %s

; void test(int64_t n, int64_t m, int64_t A[n][m]) {
;     for (int64_t i = 0; i < n; ++i) {
;         for (int64_t j = 0; j < m; ++j) {
;             A[i+2][0] = A[i][0];
;         }
;     }
; }

; CHECK: Loop: for.body: Is vectorizable with VF: 2

; Explanation: We have direction vector (<, =) with outer distance
; being greater than 1, so we can vectorize in that dinstance
; (specifically 2).

; Temporarily FAILING because we're squashing the first dimension
; (j-loop), so we're falling to innermost loop vectorization which
; we currently don't handle. Eventually, this squashing gives us more
; power.

define void @test(i64 %n, i64 %m, i64* %A) {
entry:
  %cmp3 = icmp sgt i64 %n, 0
  br i1 %cmp3, label %for.body, label %for.end9

for.body:                                         ; preds = %entry, %for.inc7
  %i.04 = phi i64 [ %inc8, %for.inc7 ], [ 0, %entry ]
  %cmp21 = icmp sgt i64 %m, 0
  br i1 %cmp21, label %for.body3, label %for.inc7

for.body3:                                        ; preds = %for.body, %for.body3
  %j.02 = phi i64 [ %inc, %for.body3 ], [ 0, %for.body ]
  %0 = mul nsw i64 %i.04, %m
  %arrayidx = getelementptr inbounds i64, i64* %A, i64 %0
  %1 = load i64, i64* %arrayidx, align 8
  %add = add nuw nsw i64 %i.04, 2
  %2 = mul nsw i64 %add, %m
  %arrayidx5 = getelementptr inbounds i64, i64* %A, i64 %2
  store i64 %1, i64* %arrayidx5, align 8
  %inc = add nuw nsw i64 %j.02, 1
  %cmp2 = icmp slt i64 %inc, %m
  br i1 %cmp2, label %for.body3, label %for.inc7

for.inc7:                                         ; preds = %for.body, %for.body3
  %inc8 = add nuw nsw i64 %i.04, 1
  %cmp = icmp slt i64 %inc8, %n
  br i1 %cmp, label %for.body, label %for.end9

for.end9:                                         ; preds = %for.inc7, %entry
  ret void
}