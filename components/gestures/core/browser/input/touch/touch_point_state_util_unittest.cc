// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/input/touch/touch_point_state_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(TouchPointStateUtilTest, IsActiveForPressed) {
  EXPECT_TRUE(IsActiveTouchState(blink::WebTouchPoint::State::kStatePressed));
}

TEST(TouchPointStateUtilTest, IsActiveForMoved) {
  EXPECT_TRUE(IsActiveTouchState(blink::WebTouchPoint::State::kStateMoved));
}

TEST(TouchPointStateUtilTest, IsActiveForStationary) {
  EXPECT_TRUE(
      IsActiveTouchState(blink::WebTouchPoint::State::kStateStationary));
}

TEST(TouchPointStateUtilTest, IsNotActiveForReleased) {
  EXPECT_FALSE(IsActiveTouchState(blink::WebTouchPoint::State::kStateReleased));
}

TEST(TouchPointStateUtilTest, IsNotActiveForCancelled) {
  EXPECT_FALSE(
      IsActiveTouchState(blink::WebTouchPoint::State::kStateCancelled));
}

TEST(TouchPointStateUtilTest, IsNotActiveForUndefined) {
  EXPECT_FALSE(
      IsActiveTouchState(blink::WebTouchPoint::State::kStateUndefined));
}

}  // namespace gestures
