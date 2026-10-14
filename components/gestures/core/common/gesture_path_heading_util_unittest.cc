// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_heading_util.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathHeadingUtilTest, SetsHeadingAndMaxForwardTowardANewPoint) {
  std::optional<gfx::Vector2dF> heading;
  float max_forward = 0.0F;

  StartGesturePathHeading(gfx::PointF(0.0, 0.0), gfx::PointF(30.0, 40.0),
                          heading, max_forward);

  EXPECT_THAT(heading, testing::Optional(gfx::Vector2dF(0.6F, 0.8F)));
  EXPECT_FLOAT_EQ(max_forward, 50.0F);
}

TEST(GesturePathHeadingUtilTest, LeavesHeadingUnsetWhenPointsCoincide) {
  std::optional<gfx::Vector2dF> heading = gfx::Vector2dF(1.0, 0.0);
  float max_forward = 10.0F;

  StartGesturePathHeading(gfx::PointF(5.0, 5.0), gfx::PointF(5.0, 5.0), heading,
                          max_forward);

  EXPECT_FALSE(heading);
}

}  // namespace gestures
