// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_classifier.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldReturnNoStrokeBelowMinimumMovement) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);

  EXPECT_FALSE(
      classifier.AddPoint(gfx::PointF(5, 0), DirectionalGestureStrokes{})
          .committed_stroke_type);
}

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldNotRecognizeStrokeFromSinglePointBeyondThreshold) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);

  // Clears `min_stroke_pixels`, but a candidate direction must still sustain
  // on its own over a second point before it commits.
  EXPECT_FALSE(
      classifier.AddPoint(gfx::PointF(30, 0), DirectionalGestureStrokes{})
          .committed_stroke_type);
}

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldRecognizeStrokeWhenDirectionPersists) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);
  classifier.AddPoint(gfx::PointF(30, 0), DirectionalGestureStrokes{});

  EXPECT_THAT(
      classifier.AddPoint(gfx::PointF(60, 0), DirectionalGestureStrokes{})
          .committed_stroke_type,
      testing::Optional(DirectionalGestureStrokeType::kRight));
}

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldRequireConsistentMovementAfterDirectionChange) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);
  // Proposes `kRight` as the pending candidate.
  classifier.AddPoint(gfx::PointF(30, 0), DirectionalGestureStrokes{});
  // Diverges to `kDown` before `kRight` sustains; the new candidate must
  // restart the sustain check, not inherit `kRight`'s progress.
  classifier.AddPoint(gfx::PointF(30, 30), DirectionalGestureStrokes{});

  EXPECT_FALSE(
      classifier.AddPoint(gfx::PointF(30, 60), DirectionalGestureStrokes{})
          .committed_stroke_type);
}

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldNotAddStrokeForContinuedMovementInSameDirection) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);
  classifier.AddPoint(gfx::PointF(30, 0), DirectionalGestureStrokes{});
  std::optional<DirectionalGestureStrokeType> stroke_type =
      classifier.AddPoint(gfx::PointF(60, 0), DirectionalGestureStrokes{})
          .committed_stroke_type;
  ASSERT_TRUE(stroke_type);

  // A straight continuation on the already-committed `kRight` direction is
  // absorbed into the existing stroke instead of committing a second one.
  EXPECT_FALSE(classifier.AddPoint(gfx::PointF(90, 0), {*stroke_type})
                   .committed_stroke_type);
}

TEST(DirectionalGestureStrokeClassifierTest,
     ShouldIgnoreIncidentalLoopBackToPreviousDirection) {
  DirectionalGestureStrokeClassifier classifier(
      /*start_location=*/gfx::PointF(0, 0), /*min_stroke_pixels=*/30,
      /*min_stroke_straightness=*/0.9);
  classifier.AddPoint(gfx::PointF(30, 0), DirectionalGestureStrokes{});
  std::optional<DirectionalGestureStrokeType> stroke_type =
      classifier.AddPoint(gfx::PointF(60, 0), DirectionalGestureStrokes{})
          .committed_stroke_type;
  ASSERT_TRUE(stroke_type);

  // A wandering ~108px detour back to a point only 30px away, well under
  // `min_stroke_straightness`, must not count as a genuine continuation.
  classifier.AddPoint(gfx::PointF(60, 50), {*stroke_type});
  EXPECT_FALSE(classifier.AddPoint(gfx::PointF(90, 0), {*stroke_type})
                   .committed_stroke_type);
}

}  // namespace gestures
