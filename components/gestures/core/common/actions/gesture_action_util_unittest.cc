// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GestureActionUtilTest, ShouldSetCommandWhenCreatingAction) {
  EXPECT_EQ(test::kFakeGestureCommand,
            MakeGestureAction(test::kFakeGestureCommand).command);
}

TEST(GestureActionUtilTest, ShouldLeaveParametersUnsetWhenCreatingAction) {
  EXPECT_TRUE(std::holds_alternative<std::monostate>(
      MakeGestureAction(test::kFakeGestureCommand).command_params));
}

}  // namespace gestures
