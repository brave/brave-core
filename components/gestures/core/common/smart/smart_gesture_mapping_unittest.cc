// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_open_in_command_param_types.h"
#include "brave/components/gestures/core/common/actions/smart_gesture_selected_text_search_command_param_types.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_key_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(SmartGestureMappingTest,
     ShouldMapLinkSwipeUpToOpenInNewBackgroundTabByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                 SmartGestureDirectionType::kUp),
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewBackgroundTab)));
}

TEST(SmartGestureMappingTest,
     ShouldMapSelectedTextSwipeUpToBraveSearchByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kSelectedText,
                                 SmartGestureDirectionType::kUp),
      testing::ElementsAre(test::MakeSearchSelectedTextGestureAction(
          SmartGestureSelectedTextSearchCommandParamType::kBraveSearch)));
}

TEST(SmartGestureMappingTest, ShouldMapImageSwipeLeftToSaveImageByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kImage,
                                         SmartGestureDirectionType::kLeft),
              testing::ElementsAre(GestureActionInfo{.command = kSaveImage}));
}

TEST(SmartGestureMappingTest,
     ShouldMapLinkSwipeRightToCopyLinkAddressByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                 SmartGestureDirectionType::kRight),
      testing::ElementsAre(GestureActionInfo{.command = kCopyLinkAddress}));
}

TEST(SmartGestureMappingTest,
     ShouldMapSelectedTextSwipeRightToCopySelectedTextByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kSelectedText,
                                 SmartGestureDirectionType::kRight),
      testing::ElementsAre(GestureActionInfo{.command = kCopySelectedText}));
}

TEST(SmartGestureMappingTest, ShouldMapImageSwipeRightToCopyImageByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kImage,
                                         SmartGestureDirectionType::kRight),
              testing::ElementsAre(GestureActionInfo{.command = kCopyImage}));
}

TEST(SmartGestureMappingTest,
     ShouldMapSelectedTextSwipeDownToAskBraveByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kSelectedText,
                                         SmartGestureDirectionType::kDown),
              testing::ElementsAre(test::MakeSearchSelectedTextGestureAction(
                  SmartGestureSelectedTextSearchCommandParamType::kAskBrave)));
}

TEST(SmartGestureMappingTest,
     ShouldMapLinkSwipeDownToOpenInNewWindowByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                 SmartGestureDirectionType::kDown),
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewWindow)));
}

TEST(SmartGestureMappingTest,
     ShouldMapImageSwipeDownToOpenInNewWindowByDefault) {
  SmartGestureMapping gesture_mapping;

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kImage,
                                 SmartGestureDirectionType::kDown),
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenImageIn, SmartGestureOpenInCommandParamType::kNewWindow)));
}

TEST(SmartGestureMappingTest, ShouldReturnNoActionsForUnassignedSlot) {
  SmartGestureMapping gesture_mapping;

  // `selected_text:left` has no default (see `DefaultSmartGestureMappings`).
  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kSelectedText,
                                         SmartGestureDirectionType::kLeft),
              testing::IsEmpty());
}

TEST(SmartGestureMappingTest, ShouldReplaceActionsWhenUpdatingMapping) {
  SmartGestureMapping gesture_mapping;
  const std::string gesture_key = ToSmartGestureKey(
      SmartGestureContentType::kLink, SmartGestureDirectionType::kLeft);

  gesture_mapping.SetActions(gesture_key,
                             {MakeGestureAction(kCopyLinkAddress)});

  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                         SmartGestureDirectionType::kLeft),
              testing::ElementsAre(MakeGestureAction(kCopyLinkAddress)));
}

TEST(SmartGestureMappingTest, ShouldRemoveMappingWhenDeleted) {
  SmartGestureMapping gesture_mapping;
  const std::string gesture_key = ToSmartGestureKey(
      SmartGestureContentType::kLink, SmartGestureDirectionType::kUp);

  gesture_mapping.Remove(gesture_key);

  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                         SmartGestureDirectionType::kUp),
              testing::IsEmpty());
}

TEST(SmartGestureMappingTest, ShouldRestoreRemovedMappingWhenResetToDefaults) {
  SmartGestureMapping gesture_mapping;
  const std::string gesture_key = ToSmartGestureKey(
      SmartGestureContentType::kLink, SmartGestureDirectionType::kUp);
  gesture_mapping.Remove(gesture_key);

  gesture_mapping.ResetToDefaults();

  EXPECT_THAT(
      gesture_mapping.ActionsFor(SmartGestureContentType::kLink,
                                 SmartGestureDirectionType::kUp),
      testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
          kOpenLinkIn, SmartGestureOpenInCommandParamType::kNewBackgroundTab)));
}

TEST(SmartGestureMappingTest,
     ShouldKeepMappingsSeparateForDifferentContentTypesWithSameDirection) {
  SmartGestureMapping gesture_mapping;

  // `link:up` and `image:up` are separate slots; `image:up`'s own default
  // (Open Image In) must not pick up `link:up`'s (Open Link In).
  EXPECT_THAT(gesture_mapping.ActionsFor(SmartGestureContentType::kImage,
                                         SmartGestureDirectionType::kUp),
              testing::ElementsAre(test::MakeGestureActionOpenInCommandParam(
                  kOpenImageIn,
                  SmartGestureOpenInCommandParamType::kNewBackgroundTab)));
}

}  // namespace gestures
