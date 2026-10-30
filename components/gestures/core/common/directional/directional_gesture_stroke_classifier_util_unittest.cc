// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureStrokeClassifierUtilTest,
     ShouldRecognizeSecondStrokeAfterSustainedDirectionChange) {
  // A 90-degree turn after a committed stroke must commit as its own
  // stroke, not get absorbed, even with a brief overshoot past the corner.
  const GesturePath gesture_path = {
      gfx::PointF(0, 0),    gfx::PointF(10, 0),   gfx::PointF(20, 0),
      gfx::PointF(30, 0),   gfx::PointF(40, 0),   gfx::PointF(50, 0),
      gfx::PointF(60, 0),   gfx::PointF(70, 0),   gfx::PointF(80, 0),
      gfx::PointF(90, 0),   gfx::PointF(100, 0),  gfx::PointF(100, 10),
      gfx::PointF(100, 20), gfx::PointF(100, 30), gfx::PointF(100, 40),
      gfx::PointF(100, 50), gfx::PointF(100, 60)};

  EXPECT_THAT(
      ClassifyDirectionalGestureStrokes(gesture_path, /*min_stroke_pixels=*/30,
                                        /*min_stroke_straightness=*/0.9,
                                        /*max_strokes=*/4),
      testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                           DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureStrokeClassifierUtilTest,
     ShouldNotRecognizeWanderingCurveAsStroke) {
  // A gesture_path that keeps changing candidate direction must not let a
  // short, locally-straight tail "launder" the curve that preceded it.
  const GesturePath gesture_path = {
      gfx::PointF(0, 0),   gfx::PointF(10, 2),  gfx::PointF(18, 8),
      gfx::PointF(22, 18), gfx::PointF(20, 28), gfx::PointF(12, 35),
      gfx::PointF(2, 36),  gfx::PointF(-6, 30), gfx::PointF(-8, 20),
      gfx::PointF(-4, 10), gfx::PointF(10, 8),  gfx::PointF(25, 8),
      gfx::PointF(40, 8)};

  EXPECT_THAT(
      ClassifyDirectionalGestureStrokes(gesture_path, /*min_stroke_pixels=*/30,
                                        /*min_stroke_straightness=*/0.9,
                                        /*max_strokes=*/4),
      testing::IsEmpty());
}

TEST(DirectionalGestureStrokeClassifierUtilTest,
     ShouldNotExtendStrokeWhenPathLoopsBack) {
  // A loop swinging back through `kRight`'s octant must not "continue"
  // that stroke; its arc length still charges against straightness.
  const GesturePath gesture_path = {
      gfx::PointF(0, 0),     gfx::PointF(30, 0),    gfx::PointF(60, 0),
      gfx::PointF(60, 50),   gfx::PointF(110, 50),  gfx::PointF(90, 0),
      gfx::PointF(90, 1000), gfx::PointF(90, 1500), gfx::PointF(90, 2000)};

  EXPECT_THAT(
      ClassifyDirectionalGestureStrokes(gesture_path, /*min_stroke_pixels=*/30,
                                        /*min_stroke_straightness=*/0.9,
                                        /*max_strokes=*/4),
      testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                           DirectionalGestureStrokeType::kDown));
}

}  // namespace gestures
