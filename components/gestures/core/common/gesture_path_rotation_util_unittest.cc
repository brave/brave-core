// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_rotation_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathRotationUtilTest, RotatesPointsToStartAtTheGivenOffset) {
  const GesturePath gesture_path = {
      gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 0.0), gfx::PointF(1.0, 1.0),
      gfx::PointF(0.0, 1.0)};

  EXPECT_THAT(
      RotateGesturePath(gesture_path, /*offset=*/2),
      testing::ElementsAre(gfx::PointF(1.0, 1.0), gfx::PointF(0.0, 1.0),
                           gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 0.0)));
}

TEST(GesturePathRotationUtilTest, LeavesAZeroOffsetUnchanged) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(1.0, 0.0)};

  EXPECT_THAT(RotateGesturePath(gesture_path, /*offset=*/0),
              testing::ElementsAreArray(gesture_path));
}

TEST(GesturePathRotationUtilTest, AveragesPerPointDistanceBetweenTwoPaths) {
  EXPECT_FLOAT_EQ(AverageGesturePathDistance(
                      {gfx::PointF(0.0, 0.0), gfx::PointF(10.0, 0.0)},
                      {gfx::PointF(0.0, 3.0), gfx::PointF(10.0, 4.0)}),
                  3.5F);
}

TEST(GesturePathRotationUtilTest, ReturnsZeroDistanceForIdenticalPaths) {
  const GesturePath gesture_path = {gfx::PointF(5.0, 5.0),
                                    gfx::PointF(10.0, 10.0)};

  EXPECT_FLOAT_EQ(AverageGesturePathDistance(gesture_path, gesture_path), 0.0F);
}

}  // namespace gestures
