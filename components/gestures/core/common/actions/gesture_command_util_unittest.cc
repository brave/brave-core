// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/actions/gesture_command_util.h"

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// Mirrors the real `kMinRealCommandId` in gesture_command_util.cc, which
// this test can't reference since it's in that file's anonymous namespace.
constexpr int kMinRealCommandId = 30000;

}  // namespace

TEST(GestureCommandUtilTest, ShouldAcceptAllSpecialCommandValues) {
  EXPECT_TRUE(IsValidGestureCommand(kUnassigned));
  EXPECT_TRUE(IsValidGestureCommand(kNavigateToUrl));
  EXPECT_TRUE(IsValidGestureCommand(kSwitchToTab));
  EXPECT_TRUE(IsValidGestureCommand(kSearchSelectedText));
  EXPECT_TRUE(IsValidGestureCommand(kOpenLinkIn));
  EXPECT_TRUE(IsValidGestureCommand(kCopyLinkAddress));
  EXPECT_TRUE(IsValidGestureCommand(kCopySelectedText));
  EXPECT_TRUE(IsValidGestureCommand(kOpenImageIn));
  EXPECT_TRUE(IsValidGestureCommand(kSaveImage));
  EXPECT_TRUE(IsValidGestureCommand(kCopyImage));
}

TEST(GestureCommandUtilTest,
     ShouldRejectValuesBetweenSpecialAndBrowserCommands) {
  EXPECT_FALSE(IsValidGestureCommand(kCopyImage + 1));
}

TEST(GestureCommandUtilTest, ShouldRejectNegativeCommandValues) {
  EXPECT_FALSE(IsValidGestureCommand(kUnassigned - 1));
}

TEST(GestureCommandUtilTest, ShouldAcceptValuesInBrowserCommandIdRange) {
  EXPECT_TRUE(IsValidGestureCommand(kMinRealCommandId));
  EXPECT_TRUE(IsValidGestureCommand(kMinRealCommandId + 10000));
}

}  // namespace gestures
