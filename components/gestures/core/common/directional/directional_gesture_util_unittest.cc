// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// `ResampleGesturePathAtInterval`'s division-based interpolation can land
// slightly off an exact round number like `25.0`.
MATCHER_P(PointFNear, expected, "") {
  return arg.IsWithinDistance(expected, 0.01F);
}

}  // namespace

TEST(DirectionalGestureUtilTest, ShouldRejectPointsCloserThanMinimumDistance) {
  EXPECT_FALSE(
      IsFarEnoughFromLastDrawnPoint(/*last_accepted_point=*/gfx::PointF(0, 0),
                                    /*candidate=*/gfx::PointF(0, 1),
                                    /*min_point_distance_pixels=*/2));
}

TEST(DirectionalGestureUtilTest, ShouldAcceptPointsAtOrBeyondMinimumDistance) {
  EXPECT_TRUE(
      IsFarEnoughFromLastDrawnPoint(/*last_accepted_point=*/gfx::PointF(0, 0),
                                    /*candidate=*/gfx::PointF(0, 2),
                                    /*min_point_distance_pixels=*/2));
}

TEST(DirectionalGestureUtilTest, ShouldReturnNoPointsWhenResamplingEmptyPath) {
  EXPECT_THAT(
      ResampleGesturePathAtInterval(GesturePath{}, /*interval_pixels=*/10),
      testing::IsEmpty());
}

TEST(DirectionalGestureUtilTest, ShouldPreserveSinglePointWhenResampling) {
  const GesturePath path = {gfx::PointF(5, 5)};

  EXPECT_THAT(ResampleGesturePathAtInterval(path, /*interval_pixels=*/10),
              testing::ElementsAre(gfx::PointF(5, 5)));
}

TEST(DirectionalGestureUtilTest, ShouldReturnFirstPointForNonPositiveInterval) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(100, 0)};

  EXPECT_THAT(ResampleGesturePathAtInterval(path, /*interval_pixels=*/0),
              testing::ElementsAre(gfx::PointF(0, 0)));
}

TEST(DirectionalGestureUtilTest, ShouldSpacePointsEvenlyAlongStraightLine) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(100, 0)};

  EXPECT_THAT(ResampleGesturePathAtInterval(path, /*interval_pixels=*/25),
              testing::ElementsAre(PointFNear(gfx::PointF(0, 0)),
                                   PointFNear(gfx::PointF(25, 0)),
                                   PointFNear(gfx::PointF(50, 0)),
                                   PointFNear(gfx::PointF(75, 0)),
                                   PointFNear(gfx::PointF(100, 0))));
}

TEST(DirectionalGestureUtilTest, ShouldAlwaysIncludeFinalPoint) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(90, 0)};

  EXPECT_THAT(ResampleGesturePathAtInterval(path, /*interval_pixels=*/25),
              testing::ElementsAre(PointFNear(gfx::PointF(0, 0)),
                                   PointFNear(gfx::PointF(25, 0)),
                                   PointFNear(gfx::PointF(50, 0)),
                                   PointFNear(gfx::PointF(75, 0)),
                                   PointFNear(gfx::PointF(90, 0))));
}

TEST(DirectionalGestureUtilTest,
     ShouldCarryOverRemainingDistanceAcrossSegments) {
  // Two original segments of 15 each, resampled at an interval of 20. The
  // leftover 15 from the first segment must carry into the second, placing
  // the single resampled point at 20 rather than independently restarting
  // accumulation at each original segment boundary.
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(15, 0),
                            gfx::PointF(30, 0)};

  EXPECT_THAT(ResampleGesturePathAtInterval(path, /*interval_pixels=*/20),
              testing::ElementsAre(PointFNear(gfx::PointF(0, 0)),
                                   PointFNear(gfx::PointF(20, 0)),
                                   PointFNear(gfx::PointF(30, 0))));
}

}  // namespace gestures
