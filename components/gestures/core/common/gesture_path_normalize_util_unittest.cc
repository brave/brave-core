// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_normalize_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathNormalizeUtilTest, ScalesToFillTheLongerBoundingBoxSide) {
  const GesturePath gesture_path = {gfx::PointF(0.0, 0.0),
                                    gfx::PointF(50.0, 100.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(0.5F, 1.0F)));
}

TEST(GesturePathNormalizeUtilTest, TranslatesSoTheBoundingBoxStartsAtOrigin) {
  const GesturePath gesture_path = {gfx::PointF(10.0, 10.0),
                                    gfx::PointF(20.0, 10.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(1.0, 0.0)));
}

TEST(GesturePathNormalizeUtilTest, CollapsesAZeroSizeBoundingBoxToTheOrigin) {
  const GesturePath gesture_path = {gfx::PointF(5.0, 5.0),
                                    gfx::PointF(5.0, 5.0)};

  EXPECT_THAT(
      NormalizeGesturePathScaleAndTranslation(gesture_path),
      testing::ElementsAre(gfx::PointF(0.0, 0.0), gfx::PointF(0.0, 0.0)));
}

}  // namespace gestures
