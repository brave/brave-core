// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(DefaultGestureMappingsTest, ShouldMapSingleStrokeKeyToDefaultCommand) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  const auto iter = default_mappings.find("↓");
  ASSERT_NE(default_mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions, testing::ElementsAre(testing::FieldsAre(
                           test::kFakeNewTab, /*command_params=*/testing::_)));
}

TEST(DefaultGestureMappingsTest, ShouldMapTwoStrokeKeyToDefaultCommand) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  const auto iter = default_mappings.find("↓→");
  ASSERT_NE(default_mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions,
              testing::ElementsAre(testing::FieldsAre(
                  test::kFakeCloseTab, /*command_params=*/testing::_)));
}

TEST(DefaultGestureMappingsTest, ShouldReturnNoEntryForUnmappedKey) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  EXPECT_FALSE(default_mappings.contains("↗"));
}

TEST(DefaultGestureMappingsTest, ShouldAssignExactlyOneActionPerMapping) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  EXPECT_THAT(default_mappings,
              testing::Each(testing::Pair(testing::_, testing::SizeIs(1))));
}

}  // namespace gestures
