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

TEST(DirectionalGestureMappingTest, MapsDefaultLeftStrokeToBack) {
  DirectionalGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kBack))));
}

TEST(DirectionalGestureMappingTest, MapsDefaultDownRightStrokeToCloseTab) {
  DirectionalGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kDown,
                      DirectionalGestureDirectionType::kRight}),
              testing::ElementsAre(testing::Eq(
                  MakeGestureAction(GestureCommandType::kCloseTab))));
}

TEST(DirectionalGestureMappingTest, ReturnsEmptyForUnmappedSequence) {
  DirectionalGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp,
                      DirectionalGestureDirectionType::kUp}),
              testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ReturnsEmptyForEmptyStrokeSequence) {
  DirectionalGestureMapping gesture_mapping;

  EXPECT_THAT(gesture_mapping.ActionsForStrokes({}), testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, SetChainedActionsOverwritesExistingKey) {
  DirectionalGestureMapping gesture_mapping;

  gesture_mapping.SetChainedActions(
      /*gesture_key=*/"←", {MakeGestureAction(GestureCommandType::kNewTab)});

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kNewTab))));
}

TEST(DirectionalGestureMappingTest, RemoveClearsTheGesture) {
  DirectionalGestureMapping gesture_mapping;

  gesture_mapping.Remove("←");

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ResetToDefaultsRestoresRemovedMapping) {
  DirectionalGestureMapping gesture_mapping;

  gesture_mapping.Remove("←");
  gesture_mapping.ResetToDefaults();

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kBack))));
}

TEST(DirectionalGestureMappingTest, ActionsForStrokesReturnsTheSetUrl) {
  DirectionalGestureMapping gesture_mapping;

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
  DirectionalGestureMapping gesture_mapping;

  gesture_mapping.SetChainedActions(
      /*gesture_key=*/"←",
      {MakeGestureAction(GestureCommandType::kNewTab),
       test::MakeNavigateToUrlGestureAction("https://brave.com")});

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(
                  std::vector<DirectionalGestureDirectionType>{
                      DirectionalGestureDirectionType::kLeft}),
              testing::ElementsAre(
                  testing::Eq(MakeGestureAction(GestureCommandType::kNewTab)),
                  testing::Eq(test::MakeNavigateToUrlGestureAction(
                      "https://brave.com"))));
}

TEST(DirectionalGestureMappingTest, RemoveClearsAllActions) {
  DirectionalGestureMapping gesture_mapping;
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
