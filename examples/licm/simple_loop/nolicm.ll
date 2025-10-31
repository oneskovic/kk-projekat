; ModuleID = 'examples/licm/simple_loop/test.ll'
source_filename = "examples/licm/simple_loop/test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @sum(i32* noundef %arg, i32 noundef %arg1) #0 {
bb:
  %i = alloca i32*, align 8
  %i2 = alloca i32, align 4
  %i3 = alloca i32, align 4
  %i4 = alloca i32, align 4
  %i5 = alloca i32, align 4
  %i6 = alloca i32, align 4
  store i32* %arg, i32** %i, align 8
  store i32 %arg1, i32* %i2, align 4
  store i32 0, i32* %i3, align 4
  store i32 5, i32* %i4, align 4
  store i32 0, i32* %i5, align 4
  br label %bb7

bb7:                                              ; preds = %bb23, %bb
  %i8 = load i32, i32* %i5, align 4
  %i9 = load i32, i32* %i2, align 4
  %i10 = icmp slt i32 %i8, %i9
  br i1 %i10, label %bb11, label %bb26

bb11:                                             ; preds = %bb7
  %i12 = load i32, i32* %i4, align 4
  %i13 = mul nsw i32 %i12, 2
  store i32 %i13, i32* %i6, align 4
  %i14 = load i32*, i32** %i, align 8
  %i15 = load i32, i32* %i5, align 4
  %i16 = sext i32 %i15 to i64
  %i17 = getelementptr inbounds i32, i32* %i14, i64 %i16
  %i18 = load i32, i32* %i17, align 4
  %i19 = load i32, i32* %i6, align 4
  %i20 = add nsw i32 %i18, %i19
  %i21 = load i32, i32* %i3, align 4
  %i22 = add nsw i32 %i21, %i20
  store i32 %i22, i32* %i3, align 4
  br label %bb23

bb23:                                             ; preds = %bb11
  %i24 = load i32, i32* %i5, align 4
  %i25 = add nsw i32 %i24, 1
  store i32 %i25, i32* %i5, align 4
  br label %bb7, !llvm.loop !6

bb26:                                             ; preds = %bb7
  %i27 = load i32, i32* %i3, align 4
  ret i32 %i27
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
