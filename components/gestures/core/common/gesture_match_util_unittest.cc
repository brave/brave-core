// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_match_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/actions/gesture_command_constants.h"
#include "brave/components/gestures/core/common/custom/custom_gesture_recognizer.h"
#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_mapping.h"
#include "brave/components/gestures/core/common/directional/directional_gesture_types.h"
#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/gesture_match_types.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_content_info.h"
#include "brave/components/gestures/core/common/smart/smart_gesture_mapping.h"
#include "brave/components/gestures/core/common/test/fake_gesture_commands.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class GestureMatchUtilTest : public testing::Test {
 public:
  GestureMatchUtilTest() {
    // Pinned since `MatchGesture`'s smart/directional/custom matchers all
    // read these; a default change shouldn't silently alter coverage.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"SmartGestureMinDistancePixels", "0"},
         {"SmartGestureMaxDistancePixels", "180"},
         {"CustomGestureResamplePointCount", "128"},
         {"CustomGestureMaxMatchDistance", "0.14"},
         {"CustomGestureInlierPointThreshold", "0.1"},
         {"CustomGestureMinHitScore", "0.08"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(GestureMatchUtilTest, ShouldReturnSmartMatchWhenItQualifies) {
  SmartGestureMapping smart_gesture_mapping;
  const SmartGestureContentInfo smart_gesture_content{
      .type = SmartGestureContentType::kLink};
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;

  const GestureMatchInfo gesture_match =
      MatchGesture(smart_gesture_mapping, smart_gesture_content,
                   directional_gesture_mapping, custom_gesture_recognizer,
                   /*strokes=*/{}, /*gesture_path=*/{{0, 0}, {50, 0}});

  EXPECT_THAT(
      gesture_match,
      testing::AllOf(
          testing::Field(&GestureMatchInfo::match_type,
                         GestureMatchType::kSmart),
          testing::Field(&GestureMatchInfo::actions,
                         testing::ElementsAre(testing::Field(
                             &GestureActionInfo::command, kCopyLinkAddress)))));
}

TEST_F(GestureMatchUtilTest,
       ShouldPreferSmartMatchWhenOtherMatchesAlsoQualify) {
  SmartGestureMapping smart_gesture_mapping;
  const SmartGestureContentInfo smart_gesture_content{
      .type = SmartGestureContentType::kLink};
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  // Shared by all three matchers; a clean straight line a saved custom
  // gesture and a mapped directional stroke would both match strongly.
  const GesturePath gesture_path{{0, 0}, {50, 0}};
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kRight};

  const GestureMatchInfo gesture_match = MatchGesture(
      smart_gesture_mapping, smart_gesture_content, directional_gesture_mapping,
      custom_gesture_recognizer, strokes, gesture_path);

  EXPECT_EQ(GestureMatchType::kSmart, gesture_match.match_type);
}

TEST_F(GestureMatchUtilTest,
       ShouldReturnDirectionalMatchWhenNoSmartOrCustomMatch) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kLeft};

  const GestureMatchInfo gesture_match = MatchGesture(
      SmartGestureMapping(), /*smart_gesture_content=*/std::nullopt,
      directional_gesture_mapping, custom_gesture_recognizer, strokes,
      /*gesture_path=*/{{100, 0}, {0, 0}});

  EXPECT_THAT(
      gesture_match,
      testing::AllOf(testing::Field(&GestureMatchInfo::match_type,
                                    GestureMatchType::kDirectional),
                     testing::Field(&GestureMatchInfo::actions,
                                    testing::ElementsAre(
                                        MakeGestureAction(test::kFakeBack)))));
}

TEST_F(GestureMatchUtilTest, ShouldReturnCustomMatchWhenNoDirectionalMatch) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", test::MakeLetterVGesturePath(),
      {MakeGestureAction(test::kFakeGestureCommand)}));
  // No default is assigned to 4 consecutive "up" strokes.
  const DirectionalGestureStrokes strokes{
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp,
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp};

  const GestureMatchInfo gesture_match = MatchGesture(
      SmartGestureMapping(), /*smart_gesture_content=*/std::nullopt,
      directional_gesture_mapping, custom_gesture_recognizer, strokes,
      test::MakeLetterVGesturePath());

  EXPECT_THAT(
      gesture_match,
      testing::AllOf(testing::Field(&GestureMatchInfo::match_type,
                                    GestureMatchType::kCustom),
                     testing::Field(&GestureMatchInfo::actions,
                                    testing::ElementsAre(MakeGestureAction(
                                        test::kFakeGestureCommand)))));
}

TEST_F(GestureMatchUtilTest,
       ShouldPreferCustomWhenClearlyCloserThanDirectional) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  // A slight bow away from the canonical straight line, so the directional
  // match still qualifies but with a nonzero distance.
  const GesturePath gesture_path{{100, 0}, {60, 5}, {0, 0}};
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kLeft};

  const GestureMatchInfo gesture_match = MatchGesture(
      SmartGestureMapping(), /*smart_gesture_content=*/std::nullopt,
      directional_gesture_mapping, custom_gesture_recognizer, strokes,
      gesture_path);

  EXPECT_THAT(
      gesture_match,
      testing::AllOf(testing::Field(&GestureMatchInfo::match_type,
                                    GestureMatchType::kCustom),
                     testing::Field(&GestureMatchInfo::actions,
                                    testing::ElementsAre(MakeGestureAction(
                                        test::kFakeGestureCommand)))));
}

TEST_F(GestureMatchUtilTest,
       ShouldPreferDirectionalWhenCustomIsNotClearlyCloser) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  const GesturePath gesture_path{{100, 0}, {0, 0}};
  // Same shape as the probe path, so its distance ties the directional
  // match's; a tie must not let custom win.
  custom_gesture_recognizer.Add(test::MakeCustomGesture(
      /*id=*/"id", gesture_path,
      {MakeGestureAction(test::kFakeGestureCommand)}));
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kLeft};

  const GestureMatchInfo gesture_match = MatchGesture(
      SmartGestureMapping(), /*smart_gesture_content=*/std::nullopt,
      directional_gesture_mapping, custom_gesture_recognizer, strokes,
      gesture_path);

  EXPECT_THAT(
      gesture_match,
      testing::AllOf(testing::Field(&GestureMatchInfo::match_type,
                                    GestureMatchType::kDirectional),
                     testing::Field(&GestureMatchInfo::actions,
                                    testing::ElementsAre(
                                        MakeGestureAction(test::kFakeBack)))));
}

TEST_F(GestureMatchUtilTest, ShouldReturnUnmatchedWhenNothingMatches) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  CustomGestureRecognizer custom_gesture_recognizer;
  const DirectionalGestureStrokes strokes{
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp,
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp};

  const GestureMatchInfo gesture_match = MatchGesture(
      SmartGestureMapping(), /*smart_gesture_content=*/std::nullopt,
      directional_gesture_mapping, custom_gesture_recognizer, strokes,
      test::MakeLetterVGesturePath());

  EXPECT_EQ(GestureMatchType::kUnmatched, gesture_match.match_type);
  EXPECT_THAT(gesture_match.actions, testing::IsEmpty());
}

}  // namespace gestures
