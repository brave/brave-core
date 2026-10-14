// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_direction_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(SmartGestureDirectionUtilTest, ShouldClassifyRightwardMovementAsRight) {
  EXPECT_EQ(SmartGestureDirectionType::kRight,
            ClassifySmartGestureDirection({{0, 0}, {100, 10}}));
}

TEST(SmartGestureDirectionUtilTest, ShouldClassifyLeftwardMovementAsLeft) {
  EXPECT_EQ(SmartGestureDirectionType::kLeft,
            ClassifySmartGestureDirection({{100, 0}, {0, 10}}));
}

TEST(SmartGestureDirectionUtilTest, ShouldClassifyDownwardMovementAsDown) {
  EXPECT_EQ(SmartGestureDirectionType::kDown,
            ClassifySmartGestureDirection({{0, 0}, {10, 100}}));
}

TEST(SmartGestureDirectionUtilTest, ShouldClassifyUpwardMovementAsUp) {
  EXPECT_EQ(SmartGestureDirectionType::kUp,
            ClassifySmartGestureDirection({{0, 100}, {10, 0}}));
}

TEST(SmartGestureDirectionUtilTest,
     ShouldPreferHorizontalAxisWhenMovementIsEqualOnBothAxes) {
  EXPECT_EQ(SmartGestureDirectionType::kRight,
            ClassifySmartGestureDirection({{0, 0}, {50, 50}}));
}

TEST(SmartGestureDirectionUtilTest, ShouldClassifyZeroMovementAsRight) {
  EXPECT_EQ(SmartGestureDirectionType::kRight,
            ClassifySmartGestureDirection({{10, 10}, {10, 10}}));
}

}  // namespace gestures
