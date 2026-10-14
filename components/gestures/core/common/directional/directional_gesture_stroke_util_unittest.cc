// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_stroke_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifyStraightRightMovementAsRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, 0),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifyDownRightMovementAsDownRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifyStraightDownMovementAsDown) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDown,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(0, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifyDownLeftMovementAsDownLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifyStraightLeftMovementAsLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, 0),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest, ShouldClassifyUpLeftMovementAsUpLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUpLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest, ShouldClassifyStraightUpMovementAsUp) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUp,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(0, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest, ShouldClassifyUpRightMovementAsUpRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUpRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldClassifySlightlyUpwardRightMovementAsRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, -0.1F),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldReturnDirectionAboveMinimumDistance) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(20, 0),
                                             /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldReturnNoDirectionBelowMinimumDistance) {
  EXPECT_FALSE(MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                                /*location=*/gfx::PointF(5, 0),
                                                /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureStrokeUtilTest, ShouldReturnDirectionAtMinimumDistance) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(10, 0),
                                             /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldReturnNoDirectionForZeroMovementEvenWhenMinimumIsZero) {
  EXPECT_FALSE(MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                                /*location=*/gfx::PointF(0, 0),
                                                /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldCombineStrokeDirectionsIntoGestureKey) {
  EXPECT_EQ("↓↑", DirectionalGestureStrokesToKey(DirectionalGestureStrokes{
                      DirectionalGestureStrokeType::kDown,
                      DirectionalGestureStrokeType::kUp}));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldKeepDiagonalKeyDistinctFromTwoStrokeKey) {
  const std::string diagonal_key = DirectionalGestureStrokesToKey(
      DirectionalGestureStrokes{DirectionalGestureStrokeType::kDownRight});
  const std::string two_stroke_key = DirectionalGestureStrokesToKey(
      DirectionalGestureStrokes{DirectionalGestureStrokeType::kDown,
                                DirectionalGestureStrokeType::kRight});

  EXPECT_NE(diagonal_key, two_stroke_key);
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldRecognizeSecondStrokeAfterSustainedDirectionChange) {
  // A clean 90-degree turn following an already-committed stroke must
  // commit as its own stroke, not get absorbed into the first one. Includes
  // a brief overshoot (75,0)/(100,10)/(100,20) that never independently
  // reclears `min_stroke_pixels` from the first stroke's own origin, the
  // exact condition that used to contaminate the second stroke's
  // straightness with leftover travel belonging to the first.
  const GesturePath path = {
      gfx::PointF(0, 0),    gfx::PointF(10, 0),   gfx::PointF(20, 0),
      gfx::PointF(30, 0),   gfx::PointF(40, 0),   gfx::PointF(50, 0),
      gfx::PointF(60, 0),   gfx::PointF(70, 0),   gfx::PointF(80, 0),
      gfx::PointF(90, 0),   gfx::PointF(100, 0),  gfx::PointF(100, 10),
      gfx::PointF(100, 20), gfx::PointF(100, 30), gfx::PointF(100, 40),
      gfx::PointF(100, 50), gfx::PointF(100, 60)};

  EXPECT_THAT(ClassifyDirectionalGestureStrokes(path, /*min_stroke_pixels=*/30,
                                                /*min_stroke_straightness=*/0.9,
                                                /*max_strokes=*/4),
              testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                                   DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldNotRecognizeWanderingCurveAsStroke) {
  // A path that keeps changing candidate direction before any one of them
  // sustains must not let a short, locally-straight tail "launder" the
  // curve that preceded it, since the wander's own arc length must still
  // count against the eventual exit segment's straightness.
  const GesturePath path = {
      gfx::PointF(0, 0),   gfx::PointF(10, 2),  gfx::PointF(18, 8),
      gfx::PointF(22, 18), gfx::PointF(20, 28), gfx::PointF(12, 35),
      gfx::PointF(2, 36),  gfx::PointF(-6, 30), gfx::PointF(-8, 20),
      gfx::PointF(-4, 10), gfx::PointF(10, 8),  gfx::PointF(25, 8),
      gfx::PointF(40, 8)};

  EXPECT_THAT(ClassifyDirectionalGestureStrokes(path, /*min_stroke_pixels=*/30,
                                                /*min_stroke_straightness=*/0.9,
                                                /*max_strokes=*/4),
              testing::IsEmpty());
}

TEST(DirectionalGestureStrokeUtilTest, ShouldNotExtendStrokeWhenPathLoopsBack) {
  // After a `kRight` stroke commits, a loop that happens to swing back
  // through the `kRight` octant relative to the stroke's origin must not be
  // accepted as "continuing" that stroke just because it lands in the same
  // direction, since the loop's own arc length must still be charged
  // against straightness, same as any other divergence. The long, clean
  // `kDown` run afterwards has to be long enough to outweigh the loop's
  // wander, which is
  // charged against it via `departure_origin` the same way it would be for
  // any other stroke built on top of a rejected divergence.
  const GesturePath path = {
      gfx::PointF(0, 0),     gfx::PointF(30, 0),    gfx::PointF(60, 0),
      gfx::PointF(60, 50),   gfx::PointF(110, 50),  gfx::PointF(90, 0),
      gfx::PointF(90, 1000), gfx::PointF(90, 1500), gfx::PointF(90, 2000)};

  EXPECT_THAT(ClassifyDirectionalGestureStrokes(path, /*min_stroke_pixels=*/30,
                                                /*min_stroke_straightness=*/0.9,
                                                /*max_strokes=*/4),
              testing::ElementsAre(DirectionalGestureStrokeType::kRight,
                                   DirectionalGestureStrokeType::kDown));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldBuildTemplateWithOneUnitSegmentPerStroke) {
  EXPECT_THAT(
      DirectionalGestureStrokesToCanonicalPath(
          DirectionalGestureStrokes{DirectionalGestureStrokeType::kRight,
                                    DirectionalGestureStrokeType::kDown}),
      testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(1, 0),
                           gfx::PointF(1, 1)));
}

TEST(DirectionalGestureStrokeUtilTest,
     ShouldReturnOriginForEmptyStrokeTemplate) {
  EXPECT_THAT(
      DirectionalGestureStrokesToCanonicalPath(DirectionalGestureStrokes{}),
      testing::ElementsAre(gfx::PointF(0, 0)));
}

}  // namespace gestures
