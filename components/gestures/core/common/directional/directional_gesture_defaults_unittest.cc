// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include <string_view>

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

void VerifyDefaultMappingExpectation(std::string_view key,
                                     int expected_gesture_command) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);
  const auto iter = default_mappings.find(key);
  ASSERT_NE(default_mappings.cend(), iter);
  const auto& [_, actions] = *iter;
  EXPECT_THAT(actions, testing::ElementsAre(
                           MakeGestureAction(expected_gesture_command)));
}

}  // namespace

TEST(DefaultGestureMappingsTest, ShouldDefaultDownToNewTab) {
  VerifyDefaultMappingExpectation("↓", test::kFakeNewTab);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultLeftToBack) {
  VerifyDefaultMappingExpectation("←", test::kFakeBack);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultRightToForward) {
  VerifyDefaultMappingExpectation("→", test::kFakeForward);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultUpToScrollTabToTop) {
  VerifyDefaultMappingExpectation("↑", test::kFakeScrollTabToTop);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultDownRightToCloseTab) {
  VerifyDefaultMappingExpectation("↓→", test::kFakeCloseTab);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultDownUpToNewWindow) {
  VerifyDefaultMappingExpectation("↓↑", test::kFakeNewWindow);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultLeftUpToPreviousTab) {
  VerifyDefaultMappingExpectation("←↑", test::kFakePreviousTab);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultRightDownToDuplicateTab) {
  VerifyDefaultMappingExpectation("→↓", test::kFakeDuplicateTab);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultRightUpToNextTab) {
  VerifyDefaultMappingExpectation("→↑", test::kFakeNextTab);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultUpDownToReload) {
  VerifyDefaultMappingExpectation("↑↓", test::kFakeReload);
}

TEST(DefaultGestureMappingsTest, ShouldDefaultUpRightToReopenClosedTab) {
  VerifyDefaultMappingExpectation("↑→", test::kFakeReopenClosedTab);
}

TEST(DefaultGestureMappingsTest, ShouldReturnNoEntryForUnmappedKey) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  EXPECT_FALSE(default_mappings.contains("↗"));
}

TEST(DefaultGestureMappingsTest, ShouldAssignExactlyOneActionPerMapping) {
  const auto default_mappings = DefaultMappings(test::kDefaultCommands);

  EXPECT_THAT(default_mappings,
              testing::Each(testing::Field(
                  &DirectionalGestureMappings::value_type::second,
                  testing::SizeIs(1))));
}

}  // namespace gestures
