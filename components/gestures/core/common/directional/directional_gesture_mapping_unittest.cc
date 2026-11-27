// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"

#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/test/gesture_action_test_util.h"
#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace gestures {

TEST(DirectionalGestureMappingTest, ShouldMapLeftStrokeToBackByDefault) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeBack)));
}

TEST(DirectionalGestureMappingTest,
     ShouldMapDownRightStrokeToCloseTabByDefault) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kDown,
                  DirectionalGestureStrokeType::kRight}),
              testing::ElementsAre(MakeGestureAction(test::kFakeCloseTab)));
}

TEST(DirectionalGestureMappingTest, ShouldReturnNoActionsForUnmappedSequence) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  EXPECT_THAT(
      gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
          DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp,
          DirectionalGestureStrokeType::kUp,
          DirectionalGestureStrokeType::kUp}),
      testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ShouldReturnNoActionsForEmptySequence) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  EXPECT_THAT(gesture_mapping.ActionsForStrokes({}), testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest, ShouldReplaceActionsWhenUpdatingMapping) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  gesture_mapping.SetActions(/*gesture_key=*/"←",
                             {MakeGestureAction(test::kFakeNewTab)});

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeNewTab)));
}

TEST(DirectionalGestureMappingTest, ShouldRemoveMappingWhenDeleted) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  gesture_mapping.Remove("←");

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::IsEmpty());
}

TEST(DirectionalGestureMappingTest,
     ShouldRestoreRemovedMappingWhenResetToDefaults) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  gesture_mapping.Remove("←");
  gesture_mapping.ResetToDefaults();

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(MakeGestureAction(test::kFakeBack)));
}

TEST(DirectionalGestureMappingTest,
     ShouldReturnConfiguredUrlForMatchingStrokes) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  gesture_mapping.SetActions(
      /*gesture_key=*/"←",
      {test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))});

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::ElementsAre(test::MakeNavigateToUrlGestureAction(
                  GURL("https://brave.com"))));
}

TEST(DirectionalGestureMappingTest,
     ShouldReturnAllChainedActionsForMatchingStrokes) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);

  gesture_mapping.SetActions(
      /*gesture_key=*/"←",
      {MakeGestureAction(test::kFakeNewTab),
       test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))});

  EXPECT_THAT(
      gesture_mapping.ActionsForStrokes(
          DirectionalGestureStrokes{DirectionalGestureStrokeType::kLeft}),
      testing::ElementsAre(
          MakeGestureAction(test::kFakeNewTab),
          test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))));
}

TEST(DirectionalGestureMappingTest, ShouldClearAllActionsWhenMappingIsRemoved) {
  DirectionalGestureMapping gesture_mapping(test::kDefaultCommands);
  gesture_mapping.SetActions(
      /*gesture_key=*/"←",
      {test::MakeNavigateToUrlGestureAction(GURL("https://brave.com"))});
  gesture_mapping.Remove("←");

  EXPECT_THAT(gesture_mapping.ActionsForStrokes(DirectionalGestureStrokes{
                  DirectionalGestureStrokeType::kLeft}),
              testing::IsEmpty());
}

}  // namespace gestures
