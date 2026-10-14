// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_key_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DirectionalGestureKeyUtilTest,
     ShouldCombineStrokeDirectionsIntoGestureKey) {
  EXPECT_EQ("↓↑", ToDirectionalGestureStrokesKey(DirectionalGestureStrokes{
                      DirectionalGestureStrokeType::kDown,
                      DirectionalGestureStrokeType::kUp}));
}

TEST(DirectionalGestureKeyUtilTest,
     ShouldDistinguishDiagonalStrokeFromTwoStrokeSequence) {
  const std::string diagonal_key = ToDirectionalGestureStrokesKey(
      DirectionalGestureStrokes{DirectionalGestureStrokeType::kDownRight});
  const std::string two_stroke_key = ToDirectionalGestureStrokesKey(
      DirectionalGestureStrokes{DirectionalGestureStrokeType::kDown,
                                DirectionalGestureStrokeType::kRight});

  EXPECT_NE(diagonal_key, two_stroke_key);
}

}  // namespace gestures
