// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_vector_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureVectorUtilTest, ResolvesCardinalDirections) {
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/0));
  EXPECT_EQ(DirectionalGestureStrokeType::kDown,
            DirectionFromDisplacement(/*dx=*/0, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/0));
  EXPECT_EQ(DirectionalGestureStrokeType::kUp,
            DirectionFromDisplacement(/*dx=*/0, /*dy=*/-1));
}

TEST(DirectionalGestureVectorUtilTest, ResolvesDiagonalDirections) {
  EXPECT_EQ(DirectionalGestureStrokeType::kDownRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kDownLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/1));
  EXPECT_EQ(DirectionalGestureStrokeType::kUpLeft,
            DirectionFromDisplacement(/*dx=*/-1, /*dy=*/-1));
  EXPECT_EQ(DirectionalGestureStrokeType::kUpRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/-1));
}

TEST(DirectionalGestureVectorUtilTest, RoundsToTheNearestOctant) {
  // Just inside the right octant's upper boundary (22.5 degrees), still
  // resolves to `kRight` rather than rounding up to `kUpRight`.
  EXPECT_EQ(DirectionalGestureStrokeType::kRight,
            DirectionFromDisplacement(/*dx=*/1, /*dy=*/-0.4F));
}

TEST(DirectionalGestureVectorUtilTest,
     ReturnsTheUnitVectorForEachCardinalDirection) {
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
     ReturnsAUnitLengthVectorForEachDiagonalDirection) {
  EXPECT_FLOAT_EQ(
      1.0F, UnitVectorForDirection(DirectionalGestureStrokeType::kDownRight)
                .Length());
  EXPECT_FLOAT_EQ(
      1.0F,
      UnitVectorForDirection(DirectionalGestureStrokeType::kUpLeft).Length());
}

}  // namespace gestures
