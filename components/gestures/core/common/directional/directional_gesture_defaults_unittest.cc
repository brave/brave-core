// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_defaults.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// Fake command ids, distinct from any real `IDC_*` value; which real
// command each default is in production is decided by the caller, not by
// this component.
constexpr int kFakeNewTab = 1000001;
constexpr int kFakeBack = 1000002;
constexpr int kFakeForward = 1000003;
constexpr int kFakeCloseTab = 1000004;
constexpr int kFakeNewWindow = 1000005;
constexpr int kFakePreviousTab = 1000006;
constexpr int kFakeDuplicateTab = 1000007;
constexpr int kFakeNextTab = 1000008;
constexpr int kFakeReload = 1000009;
constexpr int kFakeReopenClosedTab = 1000010;

DirectionalGestureDefaultCommands TestDefaultCommands() {
  return {
      .new_tab = kFakeNewTab,
      .back = kFakeBack,
      .forward = kFakeForward,
      .close_tab = kFakeCloseTab,
      .new_window = kFakeNewWindow,
      .previous_tab = kFakePreviousTab,
      .duplicate_tab = kFakeDuplicateTab,
      .next_tab = kFakeNextTab,
      .reload = kFakeReload,
      .reopen_closed_tab = kFakeReopenClosedTab,
  };
}

}  // namespace

TEST(DefaultGestureMappingsTest, SingleStrokeKeyMapsToItsCommand) {
  const auto mappings = DefaultMappings(TestDefaultCommands());

  const auto iter = mappings.find("↓");
  ASSERT_NE(mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions, testing::ElementsAre(testing::Field(
                           &GestureActionInfo::command, kFakeNewTab)));
}

TEST(DefaultGestureMappingsTest, TwoStrokeKeyMapsToItsCommand) {
  const auto mappings = DefaultMappings(TestDefaultCommands());

  const auto iter = mappings.find("↓→");
  ASSERT_NE(mappings.end(), iter);
  [[maybe_unused]] const auto& [key, actions] = *iter;
  EXPECT_THAT(actions, testing::ElementsAre(testing::Field(
                           &GestureActionInfo::command, kFakeCloseTab)));
}

TEST(DefaultGestureMappingsTest, UnmappedKeyHasNoEntry) {
  const auto mappings = DefaultMappings(TestDefaultCommands());

  EXPECT_FALSE(mappings.contains("↗"));
}

TEST(DefaultGestureMappingsTest, EveryEntryHasExactlyOneAction) {
  const auto mappings = DefaultMappings(TestDefaultCommands());

  EXPECT_THAT(mappings,
              testing::Each(testing::Pair(testing::_, testing::SizeIs(1))));
}

}  // namespace gestures
