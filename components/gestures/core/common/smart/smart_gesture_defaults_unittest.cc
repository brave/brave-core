// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_defaults.h"

#include <string_view>

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

void VerifyDefaultMappingExpectation(
    std::string_view key,
    const GestureActionInfo& expected_gesture_action) {
  const SmartGestureMappings default_gesture_mappings =
      DefaultSmartGestureMappings();
  const auto iter = default_gesture_mappings.find(key);
  ASSERT_NE(default_gesture_mappings.cend(), iter);
  const auto& [_, actions] = *iter;
  EXPECT_THAT(actions, testing::ElementsAre(expected_gesture_action));
}

}  // namespace

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkUpToOpenInNewBackgroundTab) {
  VerifyDefaultMappingExpectation(
      "link:up",
      test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewBackgroundTab));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkDownToOpenInNewWindow) {
  VerifyDefaultMappingExpectation(
      "link:down",
      test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewWindow));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkLeftToOpenInNewForegroundTab) {
  VerifyDefaultMappingExpectation(
      "link:left",
      test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkRightToCopyLinkAddress) {
  VerifyDefaultMappingExpectation("link:right",
                                  MakeGestureAction(kCopyLinkAddress));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultSelectedTextUpToBraveSearch) {
  VerifyDefaultMappingExpectation(
      "selected_text:up",
      test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultSelectedTextDownToAskBrave) {
  VerifyDefaultMappingExpectation(
      "selected_text:down",
      test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kAskBrave));
}

TEST(SmartGestureDefaultsTest,
     ShouldDefaultSelectedTextRightToCopySelectedText) {
  VerifyDefaultMappingExpectation("selected_text:right",
                                  MakeGestureAction(kCopySelectedText));
}

TEST(SmartGestureDefaultsTest, ShouldLeaveSelectedTextLeftUnassigned) {
  const SmartGestureMappings default_gesture_mappings =
      DefaultSmartGestureMappings();

  EXPECT_FALSE(default_gesture_mappings.contains("selected_text:left"));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageUpToOpenInNewBackgroundTab) {
  VerifyDefaultMappingExpectation(
      "image:up",
      test::MakeGestureActionOpenInCommandParam(
          kOpenImageIn, SmartGestureOpenInCommandParamType::kNewBackgroundTab));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageDownToOpenInNewWindow) {
  VerifyDefaultMappingExpectation(
      "image:down",
      test::MakeGestureActionOpenInCommandParam(
          kOpenImageIn, SmartGestureOpenInCommandParamType::kNewWindow));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageLeftToSaveImage) {
  VerifyDefaultMappingExpectation("image:left", MakeGestureAction(kSaveImage));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageRightToCopyImage) {
  VerifyDefaultMappingExpectation("image:right", MakeGestureAction(kCopyImage));
}

}  // namespace gestures
