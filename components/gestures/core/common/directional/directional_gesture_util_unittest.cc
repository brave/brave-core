// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureUtilTest, MovementStraightRightResolvesToRight) {
  EXPECT_EQ(DirectionalGestureDirectionType::kRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(1, 0),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementDownRightResolvesToDownRight) {
  EXPECT_EQ(DirectionalGestureDirectionType::kDownRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(1, 1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementStraightDownResolvesToDown) {
  EXPECT_EQ(DirectionalGestureDirectionType::kDown,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(0, 1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementDownLeftResolvesToDownLeft) {
  EXPECT_EQ(DirectionalGestureDirectionType::kDownLeft,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(-1, 1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementStraightLeftResolvesToLeft) {
  EXPECT_EQ(DirectionalGestureDirectionType::kLeft,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(-1, 0),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementUpLeftResolvesToUpLeft) {
  EXPECT_EQ(DirectionalGestureDirectionType::kUpLeft,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(-1, -1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementStraightUpResolvesToUp) {
  EXPECT_EQ(DirectionalGestureDirectionType::kUp,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(0, -1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementUpRightResolvesToUpRight) {
  EXPECT_EQ(DirectionalGestureDirectionType::kUpRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(1, -1),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest,
     MovementSlightlyAboveStraightRightResolvesToRight) {
  EXPECT_EQ(DirectionalGestureDirectionType::kRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(1, -0.1F),
                                   /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest, MovementAboveMinimumPixelsReturnsDirection) {
  EXPECT_EQ(DirectionalGestureDirectionType::kRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(20, 0),
                                   /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureUtilTest, MovementBelowMinimumPixelsReturnsNullopt) {
  EXPECT_FALSE(MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                      /*location=*/gfx::PointF(5, 0),
                                      /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureUtilTest,
     MovementExactlyAtMinimumPixelsReturnsDirection) {
  EXPECT_EQ(DirectionalGestureDirectionType::kRight,
            MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                   /*location=*/gfx::PointF(10, 0),
                                   /*min_stroke_pixels=*/10));
}

TEST(DirectionalGestureUtilTest,
     ZeroMovementReturnsNulloptEvenWithZeroMinimumPixels) {
  EXPECT_FALSE(MaybeMovementDirection(/*origin=*/gfx::PointF(0, 0),
                                      /*location=*/gfx::PointF(0, 0),
                                      /*min_stroke_pixels=*/0));
}

TEST(DirectionalGestureUtilTest,
     StrokesToGestureKeyConcatenatesDirectionCodes) {
  EXPECT_EQ("↓↑",
            StrokesToGestureKey(std::vector<DirectionalGestureDirectionType>{
                DirectionalGestureDirectionType::kDown,
                DirectionalGestureDirectionType::kUp}));
}

TEST(DirectionalGestureUtilTest,
     DiagonalGestureKeyDoesNotCollideWithTwoStrokeKey) {
  const std::string diagonal_key =
      StrokesToGestureKey(std::vector<DirectionalGestureDirectionType>{
          DirectionalGestureDirectionType::kDownRight});
  const std::string two_stroke_key =
      StrokesToGestureKey(std::vector<DirectionalGestureDirectionType>{
          DirectionalGestureDirectionType::kDown,
          DirectionalGestureDirectionType::kRight});

  EXPECT_NE(diagonal_key, two_stroke_key);
}

}  // namespace gestures
