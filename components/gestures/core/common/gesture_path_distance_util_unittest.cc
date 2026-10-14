// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_distance_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathLengthTest, SumsConsecutiveSegmentDistances) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(30, 0),
                                    gfx::PointF(30, 40)};

  EXPECT_FLOAT_EQ(GesturePathLength(gesture_path), 70.0F);
}

TEST(GesturePathLengthTest, DoesNotShrinkWhenPathCurvesBackTowardStart) {
  // Net start-to-end displacement here is small, but the gesture_path
  // traveled a long way to get there, which is what this is meant to capture.
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(100, 0),
                                    gfx::PointF(100, 100), gfx::PointF(5, 100)};

  EXPECT_FLOAT_EQ(GesturePathLength(gesture_path), 295.0F);
}

TEST(IsFarEnoughFromLastDrawnPointTest, RejectsPointsCloserThanMinimum) {
  EXPECT_FALSE(IsFarEnoughFromLastDrawnPoint(
      /*last_drawn_point=*/gfx::PointF(0, 0), /*candidate=*/gfx::PointF(0, 1),
      /*min_point_distance_pixels=*/2));
}

TEST(IsFarEnoughFromLastDrawnPointTest, AcceptsPointsAtOrBeyondMinimum) {
  EXPECT_TRUE(IsFarEnoughFromLastDrawnPoint(
      /*last_drawn_point=*/gfx::PointF(0, 0), /*candidate=*/gfx::PointF(0, 2),
      /*min_point_distance_pixels=*/2));
}

class IsClosedGesturePathTest : public testing::Test {
 public:
  IsClosedGesturePathTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"CustomGestureClosedPathGapRatio", "0.2"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(IsClosedGesturePathTest, ReturnsFalseForZeroLengthPath) {
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(0, 0)};

  EXPECT_FALSE(IsClosedGesturePath(raw_points));
}

TEST_F(IsClosedGesturePathTest, ReturnsFalseWhenTheEndIsFarFromTheStart) {
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_FALSE(IsClosedGesturePath(raw_points));
}

TEST_F(IsClosedGesturePathTest, ReturnsTrueWhenTheEndIsCloseToTheStart) {
  // Length 19.5, gap 0.5, a ratio of ~0.026, comfortably under the pinned
  // 0.2 threshold.
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(10, 0),
                                  gfx::PointF(0.5, 0)};

  EXPECT_TRUE(IsClosedGesturePath(raw_points));
}

}  // namespace gestures
