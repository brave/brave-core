// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_command_util.h"

#include <utility>

#include "brave/components/gestures/core/common/actions/gesture_command_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GestureCommandUtilTest, ValueWithinRangeIsValid) {
  EXPECT_TRUE(
      IsValidGestureCommand(std::to_underlying(GestureCommandType::kBack)));
}

TEST(GestureCommandUtilTest, MinimumValueIsValid) {
  EXPECT_TRUE(IsValidGestureCommand(
      std::to_underlying(GestureCommandType::kUnassigned)));
}

TEST(GestureCommandUtilTest, MaximumValueIsValid) {
  EXPECT_TRUE(
      IsValidGestureCommand(std::to_underlying(GestureCommandType::kMaxValue)));
}

TEST(GestureCommandUtilTest, ValueBelowTheMinimumIsInvalid) {
  EXPECT_FALSE(IsValidGestureCommand(
      std::to_underlying(GestureCommandType::kUnassigned) - 1));
}

TEST(GestureCommandUtilTest, ValueAboveTheMaximumIsInvalid) {
  EXPECT_FALSE(IsValidGestureCommand(
      std::to_underlying(GestureCommandType::kMaxValue) + 1));
}

}  // namespace gestures
