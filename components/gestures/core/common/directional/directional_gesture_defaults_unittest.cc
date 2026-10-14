// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DefaultGestureMappingsTest, SingleStrokeKeyMapsToItsCommand) {
  const auto mappings = DefaultMappings();

  const auto iter = mappings.find("↓");
  ASSERT_NE(mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions,
              testing::ElementsAre(testing::Field(
                  &GestureActionInfo::command, GestureCommandType::kNewTab)));
}

TEST(DefaultGestureMappingsTest, TwoStrokeKeyMapsToItsCommand) {
  const auto mappings = DefaultMappings();

  const auto iter = mappings.find("↓→");
  ASSERT_NE(mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions,
              testing::ElementsAre(testing::Field(
                  &GestureActionInfo::command, GestureCommandType::kCloseTab)));
}

TEST(DefaultGestureMappingsTest, UnmappedKeyHasNoEntry) {
  const auto mappings = DefaultMappings();

  EXPECT_FALSE(mappings.contains("↗"));
}

TEST(DefaultGestureMappingsTest, EveryEntryHasExactlyOneAction) {
  const auto mappings = DefaultMappings();

  EXPECT_THAT(mappings,
              testing::Each(testing::Pair(testing::_, testing::SizeIs(1))));
}

}  // namespace gestures
