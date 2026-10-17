// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// An arbitrary value standing in for a real `IDC_*` command id; this file
// doesn't depend on `IDC_*` headers.
constexpr int kSomeRealCommand = 33000;

}  // namespace

TEST(GestureActionUtilTest, MakeGestureActionSetsTheCommand) {
  EXPECT_EQ(kSomeRealCommand, MakeGestureAction(kSomeRealCommand).command);
}

TEST(GestureActionUtilTest, MakeGestureActionLeavesParamsAsMonostate) {
  EXPECT_TRUE(std::holds_alternative<std::monostate>(
      MakeGestureAction(kSomeRealCommand).command_params));
}

}  // namespace gestures
