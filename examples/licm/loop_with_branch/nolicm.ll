; ModuleID = 'examples/licm/loop_with_branch/test.ll'
source_filename = "examples/licm/loop_with_branch/test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @f(i32 noundef %arg, i32 noundef %arg1) #0 {
bb:
  %i = alloca i32, align 4
  %i2 = alloca i32, align 4
  %i3 = alloca i32, align 4
  %i4 = alloca i32, align 4
  %i5 = alloca i32, align 4
  store i32 %arg, i32* %i, align 4
  store i32 %arg1, i32* %i2, align 4
  store i32 0, i32* %i3, align 4
  store i32 0, i32* %i4, align 4
  br label %bb6

bb6:                                              ; preds = %bb27, %bb
  %i7 = load i32, i32* %i4, align 4
  %i8 = load i32, i32* %i, align 4
  %i9 = icmp slt i32 %i7, %i8
  br i1 %i9, label %bb10, label %bb30

bb10:                                             ; preds = %bb6
  %i11 = load i32, i32* %i4, align 4
  %i12 = srem i32 %i11, 2
  %i13 = icmp eq i32 %i12, 0
  br i1 %i13, label %bb14, label %bb22

bb14:                                             ; preds = %bb10
  %i15 = load i32, i32* %i2, align 4
  %i16 = mul nsw i32 %i15, 3
  store i32 %i16, i32* %i5, align 4
  %i17 = load i32, i32* %i4, align 4
  %i18 = load i32, i32* %i5, align 4
  %i19 = mul nsw i32 %i17, %i18
  %i20 = load i32, i32* %i3, align 4
  %i21 = add nsw i32 %i20, %i19
  store i32 %i21, i32* %i3, align 4
  br label %bb26

bb22:                                             ; preds = %bb10
  %i23 = load i32, i32* %i4, align 4
  %i24 = load i32, i32* %i3, align 4
  %i25 = add nsw i32 %i24, %i23
  store i32 %i25, i32* %i3, align 4
  br label %bb26

bb26:                                             ; preds = %bb22, %bb14
  br label %bb27

bb27:                                             ; preds = %bb26
  %i28 = load i32, i32* %i4, align 4
  %i29 = add nsw i32 %i28, 1
  store i32 %i29, i32* %i4, align 4
  br label %bb6, !llvm.loop !6

bb30:                                             ; preds = %bb6
  %i31 = load i32, i32* %i3, align 4
  ret i32 %i31
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
