// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureVectorUtilTest, ShouldResolveCardinalDirections) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/0));
  EXPECT_EQ(DirectionalGestureStrokeType::kDown,
            DirectionFromDisplacement(/*dx=*/0, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/0));
  EXPECT_EQ(DirectionalGestureStrokeType::kUp,
            DirectionFromDisplacement(/*dx=*/0, /*dy=*/-1));
}

TEST(DirectionalGestureVectorUtilTest, ShouldResolveDiagonalDirections) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kDownLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kUpLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/-1));
  EXPECT_EQ(DirectionalGestureStrokeType::kUpRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/-1));
}

TEST(DirectionalGestureVectorUtilTest, ShouldRoundToNearestOctant) {
  // Just inside the right octant's upper boundary (22.5 degrees), still
  // resolves to `kRight` rather than rounding up to `kUpRight`.
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/-0.4F));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldReturnUnitVectorForEachCardinalDirection) {
  EXPECT_EQ(gfx::Vector2dF(1, 0),
            UnitVectorForDirection(DirectionalGestureStrokeType::kRight));
  EXPECT_EQ(gfx::Vector2dF(0, 1),
            UnitVectorForDirection(DirectionalGestureStrokeType::kDown));
  EXPECT_EQ(gfx::Vector2dF(-1, 0),
            UnitVectorForDirection(DirectionalGestureStrokeType::kLeft));
  EXPECT_EQ(gfx::Vector2dF(0, -1),
            UnitVectorForDirection(DirectionalGestureStrokeType::kUp));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldReturnUnitLengthVectorForEachDiagonalDirection) {
  EXPECT_FLOAT_EQ(
      1.0F, UnitVectorForDirection(DirectionalGestureStrokeType::kDownRight)
                .Length());
  EXPECT_FLOAT_EQ(
      1.0F,
      UnitVectorForDirection(DirectionalGestureStrokeType::kUpLeft).Length());
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifyStraightRightMovementAsRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, 0),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifyDownRightMovementAsDownRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifyStraightDownMovementAsDown) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDown,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(0, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifyDownLeftMovementAsDownLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, 1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifyStraightLeftMovementAsLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, 0),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest, ShouldClassifyUpLeftMovementAsUpLeft) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUpLeft,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(-1, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest, ShouldClassifyStraightUpMovementAsUp) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUp,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(0, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest, ShouldClassifyUpRightMovementAsUpRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kUpRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, -1),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldClassifySlightlyUpwardRightMovementAsRight) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(1, -0.1F),
                                             /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldReturnDirectionAboveMinimumDistance) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(20, 0),
                                             /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldReturnNoDirectionBelowMinimumDistance) {
  EXPECT_FALSE(MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                                /*location=*/gfx::PointF(5, 0),
                                                /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureVectorUtilTest, ShouldReturnDirectionAtMinimumDistance) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                             /*location=*/gfx::PointF(10, 0),
                                             /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureVectorUtilTest,
     ShouldReturnNoDirectionForZeroMovementEvenWhenMinimumIsZero) {
  EXPECT_FALSE(MaybeGetDirectionalGestureStroke(/*origin=*/gfx::PointF(0, 0),
                                                /*location=*/gfx::PointF(0, 0),
                                                /*min_stroke_pixels=*/0));
}

}  // namespace gestures
