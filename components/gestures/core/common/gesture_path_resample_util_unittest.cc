// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

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

TEST(GesturePathResampleUtilTest, ResamplesToRequestedPointCount) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(100.0, 0.0)};

  EXPECT_EQ(ResampleGesturePathToPointCount(gesture_path, 5).size(), 5U);
}

TEST(GesturePathResampleUtilTest, ResamplesStraightLineWithEvenPointSpacing) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(100.0, 0.0)};

  EXPECT_THAT(
      ResampleGesturePathToPointCount(gesture_path, 3),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(50.0, 0.0),
                           gfx::PointF(100.0, 0.0)));
}

TEST(GesturePathResampleUtilTest, RepeatsSinglePointForZeroLengthPath) {
  const GesturePath gesture_path = {gfx::PointF(10.0, 10.0)};

  EXPECT_THAT(
      ResampleGesturePathToPointCount(gesture_path, 3),
      testing::ElementsAre(gfx::PointF(10.0, 10.0), gfx::PointF(10.0, 10.0),
                           gfx::PointF(10.0, 10.0)));
}

TEST(GesturePathResampleUtilTest, ShouldReturnNoPointsWhenResamplingEmptyPath) {
  EXPECT_THAT(
      ResampleGesturePathAtInterval(GesturePath{}, /*interval_pixels=*/10),
      testing::IsEmpty());
}

TEST(GesturePathResampleUtilTest, ShouldPreserveSinglePointWhenResampling) {
  const GesturePath gesture_path = {gfx::PointF(5, 5)};

  EXPECT_THAT(
      ResampleGesturePathAtInterval(gesture_path, /*interval_pixels=*/10),
      testing::ElementsAre(gfx::PointF(5, 5)));
}

TEST(GesturePathResampleUtilTest,
     ShouldReturnFirstPointForNonPositiveInterval) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(100, 0)};

  EXPECT_THAT(
      ResampleGesturePathAtInterval(gesture_path, /*interval_pixels=*/0),
      testing::ElementsAre(gfx::PointF(0, 0)));
}

TEST(GesturePathResampleUtilTest, ShouldSpacePointsEvenlyAlongStraightLine) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(100, 0)};

  EXPECT_THAT(
      ResampleGesturePathAtInterval(gesture_path, /*interval_pixels=*/25),
      testing::ElementsAre(
          PointFNear(gfx::PointF(0, 0)), PointFNear(gfx::PointF(25, 0)),
          PointFNear(gfx::PointF(50, 0)), PointFNear(gfx::PointF(75, 0)),
          PointFNear(gfx::PointF(100, 0))));
}

TEST(GesturePathResampleUtilTest, ShouldAlwaysIncludeFinalPoint) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(90, 0)};

  EXPECT_THAT(
      ResampleGesturePathAtInterval(gesture_path, /*interval_pixels=*/25),
      testing::ElementsAre(
          PointFNear(gfx::PointF(0, 0)), PointFNear(gfx::PointF(25, 0)),
          PointFNear(gfx::PointF(50, 0)), PointFNear(gfx::PointF(75, 0)),
          PointFNear(gfx::PointF(90, 0))));
}

TEST(GesturePathResampleUtilTest,
     ShouldCarryOverRemainingDistanceAcrossSegments) {
  // Two 15-length segments resampled at interval 20; the leftover 15 from
  // the first must carry into the second, not restart at the boundary.
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(15, 0),
                                    gfx::PointF(30, 0)};

  EXPECT_THAT(
      ResampleGesturePathAtInterval(gesture_path, /*interval_pixels=*/20),
      testing::ElementsAre(PointFNear(gfx::PointF(0, 0)),
                           PointFNear(gfx::PointF(20, 0)),
                           PointFNear(gfx::PointF(30, 0))));
}

}  // namespace gestures
