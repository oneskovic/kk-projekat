; ModuleID = 'examples/licm/test.ll'
source_filename = "examples/licm/test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @f(i32 noundef %arg) #0 {
bb:
  %i = alloca i32, align 4
  %i1 = alloca i32, align 4
  %i2 = alloca i32, align 4
  %i3 = alloca i32, align 4
  store i32 %arg, i32* %i, align 4
  store i32 5, i32* %i1, align 4
  store i32 0, i32* %i2, align 4
  %i6 = load i32, i32* %i, align 4
  %i12 = load i32, i32* %i1, align 4
  %i13 = mul nsw i32 %i12, 3
  br label %bb4

bb4:                                              ; preds = %bb15, %bb
  %i5 = load i32, i32* %i2, align 4
  %i7 = icmp slt i32 %i5, %i6
  br i1 %i7, label %bb8, label %bb18

bb8:                                              ; preds = %bb4
  %i9 = load i32, i32* %i2, align 4
  %i10 = icmp sgt i32 %i9, 5
  br i1 %i10, label %bb11, label %bb14

bb11:                                             ; preds = %bb8
  store i32 %i13, i32* %i3, align 4
  br label %bb14

bb14:                                             ; preds = %bb11, %bb8
  br label %bb15

bb15:                                             ; preds = %bb14
  %i16 = load i32, i32* %i2, align 4
  %i17 = add nsw i32 %i16, 1
  store i32 %i17, i32* %i2, align 4
  br label %bb4, !llvm.loop !6

bb18:                                             ; preds = %bb4
  %i19 = load i32, i32* %i1, align 4
  ret i32 %i19
}

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"Ubuntu clang version 14.0.0-1ubuntu1.1"}
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
