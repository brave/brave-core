// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_defaults.h"

#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkUpToOpenInNewBackgroundTab) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("link:up");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewBackgroundTab)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkDownToOpenInNewWindow) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("link:down");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewWindow)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkLeftToOpenInNewForegroundTab) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("link:left");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewForegroundTab)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultLinkRightToCopyLinkAddress) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("link:right");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(GestureActionInfo{.command = kCopyLinkAddress}));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultSelectedTextUpToBraveSearch) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("selected_text:up");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultSelectedTextDownToBraveAsk) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("selected_text:down");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(iter->second,
              testing::ElementsAre(test::MakeSearchSelectedTextGestureAction(
                  SmartGestureSelectedTextSearchCommandParamType::kBraveAsk)));
}

TEST(SmartGestureDefaultsTest,
     ShouldDefaultSelectedTextRightToCopySelectedText) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("selected_text:right");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(GestureActionInfo{.command = kCopySelectedText}));
}

TEST(SmartGestureDefaultsTest, ShouldLeaveSelectedTextLeftUnassigned) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  EXPECT_EQ(defaults.end(), defaults.find("selected_text:left"));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageUpToOpenInNewBackgroundTab) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("image:up");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(iter->second,
              testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
                  kOpenImageIn,
                  SmartGestureOpenInCommandParamType::kNewBackgroundTab)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageDownToOpenInNewWindow) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("image:down");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(
      iter->second,
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenImageIn, SmartGestureOpenInCommandParamType::kNewWindow)));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageLeftToSaveImage) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("image:left");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(iter->second,
              testing::ElementsAre(GestureActionInfo{.command = kSaveImage}));
}

TEST(SmartGestureDefaultsTest, ShouldDefaultImageRightToCopyImage) {
  const SmartGestureMappings defaults = DefaultSmartGestureMappings();

  const auto iter = defaults.find("image:right");
  ASSERT_NE(defaults.end(), iter);
  EXPECT_THAT(iter->second,
              testing::ElementsAre(GestureActionInfo{.command = kCopyImage}));
}

}  // namespace gestures
