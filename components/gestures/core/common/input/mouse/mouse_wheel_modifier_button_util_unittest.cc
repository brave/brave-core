// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_util.h"

#include <utility>

#include "brave/components/gestures/core/common/input/mouse/mouse_wheel_modifier_button_types.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(MouseWheelModifierButtonUtilTest, ValueWithinRangeIsValid) {
  EXPECT_TRUE(IsValidMouseWheelModifierButton(
      std::to_underlying(MouseWheelModifierButtonType::kMiddle)));
}

TEST(MouseWheelModifierButtonUtilTest, MinimumValueIsValid) {
  EXPECT_TRUE(IsValidMouseWheelModifierButton(
      std::to_underlying(MouseWheelModifierButtonType::kLeft)));
}

TEST(MouseWheelModifierButtonUtilTest, MaximumValueIsValid) {
  EXPECT_TRUE(IsValidMouseWheelModifierButton(
      std::to_underlying(MouseWheelModifierButtonType::kMaxValue)));
}

TEST(MouseWheelModifierButtonUtilTest, ValueBelowTheMinimumIsInvalid) {
  EXPECT_FALSE(IsValidMouseWheelModifierButton(
      std::to_underlying(MouseWheelModifierButtonType::kLeft) - 1));
}

TEST(MouseWheelModifierButtonUtilTest, ValueAboveTheMaximumIsInvalid) {
  EXPECT_FALSE(IsValidMouseWheelModifierButton(
      std::to_underlying(MouseWheelModifierButtonType::kMaxValue) + 1));
}

}  // namespace gestures
