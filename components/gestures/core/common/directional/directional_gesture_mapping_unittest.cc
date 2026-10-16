// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// Fake command ids, distinct from any real `IDC_*` value, for tests that
// only exercise that the mapping stores and returns what it's given; which
// real command each default is in production is decided by the caller, not
// by this component.
constexpr int kFakeBack = 1000001;
constexpr int kFakeForward = 1000002;
constexpr int kFakeNewTab = 1000003;
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

TEST(DirectionalGestureMappingTest, MapsDefaultLeftStrokeToBack) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(testing::Eq(MakeGestureAction(kFakeBack))));
}

TEST(DirectionalGestureMappingTest, MapsDefaultDownRightStrokeToCloseTab) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  EXPECT_THAT(
      gesture_mapping.ActionsForStrokes(
          std::vector<DirectionalGestureDirectionType>{
              DirectionalGestureDirectionType::kDown,
              DirectionalGestureDirectionType::kRight}),
      testing::ElementsAre(testing::Eq(MakeGestureAction(kFakeCloseTab))));
}

TEST(DirectionalGestureMappingTest, ReturnsEmptyForUnmappedSequence) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp}),
              testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ReturnsEmptyForEmptyStrokeSequence) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  EXPECT_THAT(gesture_mapping.ActionsForStrokes({}), testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, SetChainedActionsOverwritesExistingKey) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  gesture_mapping.SetChainedActions(/*gesture_key=*/"←",
                                    {MakeGestureAction(kFakeNewTab)});

  EXPECT_THAT(
      gesture_mapping.ActionsForStrokes(
          std::vector<DirectionalGestureDirectionType>{
              DirectionalGestureDirectionType::kLeft}),
      testing::ElementsAre(testing::Eq(MakeGestureAction(kFakeNewTab))));
}

TEST(DirectionalGestureMappingTest, RemoveClearsTheGesture) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  gesture_mapping.Remove("←");

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ResetToDefaultsRestoresRemovedMapping) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  gesture_mapping.Remove("←");
  gesture_mapping.ResetToDefaults();

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(testing::Eq(MakeGestureAction(kFakeBack))));
}

TEST(DirectionalGestureMappingTest, ActionsForStrokesReturnsTheSetUrl) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  gesture_mapping.SetChainedActions(
      /*gesture_key=*/"←",
      {test::MakeNavigateToUrlGestureAction("https://brave.com")});

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(testing::Eq(
                  test::MakeNavigateToUrlGestureAction("https://brave.com"))));
}

TEST(DirectionalGestureMappingTest, ActionsForStrokesReturnsAllChainedActions) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());

  gesture_mapping.SetChainedActions(
      /*gesture_key=*/"←",
      {MakeGestureAction(kFakeNewTab),
       test::MakeNavigateToUrlGestureAction("https://brave.com")});

  EXPECT_THAT(
      gesture_mapping.ActionsForStrokes(
          std::vector<DirectionalGestureDirectionType>{
              DirectionalGestureDirectionType::kLeft}),
      testing::ElementsAre(testing::Eq(MakeGestureAction(kFakeNewTab)),
                           testing::Eq(test::MakeNavigateToUrlGestureAction(
                               "https://brave.com"))));
}

TEST(DirectionalGestureMappingTest, RemoveClearsAllActions) {
  DirectionalGestureMapping gesture_mapping(TestDefaultCommands());
  gesture_mapping.SetChainedActions(
      /*gesture_key=*/"←",
      {test::MakeNavigateToUrlGestureAction("https://brave.com")});
  gesture_mapping.Remove("←");

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::IsEmpty());
}

}  // namespace gestures
