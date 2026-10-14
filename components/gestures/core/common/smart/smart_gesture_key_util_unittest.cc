// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_key_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(SmartGestureKeyUtilTest, ShouldFormatLinkUpKey) {
  EXPECT_EQ("link:up", SmartGestureKey(SmartGestureContentType::kLink,
                                       SmartGestureDirectionType::kUp));
}

TEST(SmartGestureKeyUtilTest, ShouldFormatImageDownKey) {
  EXPECT_EQ("image:down", SmartGestureKey(SmartGestureContentType::kImage,
                                          SmartGestureDirectionType::kDown));
}

TEST(SmartGestureKeyUtilTest, ShouldFormatSelectedTextLeftKey) {
  EXPECT_EQ("selected_text:left",
            SmartGestureKey(SmartGestureContentType::kSelectedText,
                            SmartGestureDirectionType::kLeft));
}

TEST(SmartGestureKeyUtilTest, ShouldFormatLinkRightKey) {
  EXPECT_EQ("link:right", SmartGestureKey(SmartGestureContentType::kLink,
                                          SmartGestureDirectionType::kRight));
}

}  // namespace gestures
