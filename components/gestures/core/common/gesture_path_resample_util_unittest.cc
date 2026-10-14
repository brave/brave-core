// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_resample_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathResampleUtilTest, ResamplesToExactlyThePointCountRequested) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(100.0, 0.0)};

  EXPECT_EQ(ResampleGesturePathToCount(gesture_path, 5).size(), 5U);
}

TEST(GesturePathResampleUtilTest, ResamplesEvenlyAlongAStraightLine) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(100.0, 0.0)};

  EXPECT_THAT(
      ResampleGesturePathToCount(gesture_path, 3),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(50.0, 0.0),
                           gfx::PointF(100.0, 0.0)));
}

TEST(GesturePathResampleUtilTest, RepeatsTheOnlyPointForAZeroLengthPath) {
  const GesturePath gesture_path = {gfx::PointF(10.0, 10.0)};

  EXPECT_THAT(
      ResampleGesturePathToCount(gesture_path, 3),
      testing::ElementsAre(gfx::PointF(10.0, 10.0), gfx::PointF(10.0, 10.0),
                           gfx::PointF(10.0, 10.0)));
}

TEST(GesturePathResampleUtilTest, ScalesToFillTheLongerBoundingBoxSide) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(50.0, 100.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(0.5F, 1.0F)));
}

TEST(GesturePathResampleUtilTest, TranslatesSoTheBoundingBoxStartsAtOrigin) {
  const GesturePath gesture_path = {gfx::PointF(10.0, 10.0),
                                    gfx::PointF(20.0, 10.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 0.0)));
}

TEST(GesturePathResampleUtilTest, CollapsesAZeroSizeBoundingBoxToTheOrigin) {
  const GesturePath gesture_path = {gfx::PointF(5.0, 5.0),
                                    gfx::PointF(5.0, 5.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(0.0, 0.0)));
}

}  // namespace gestures
