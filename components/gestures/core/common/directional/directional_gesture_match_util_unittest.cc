// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/directional/directional_gesture_match_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/actions/gesture_action_util.h"
#include "brave/components/gestures/core/common/directional/test/directional_gesture_default_commands_test_constants.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class DirectionalGestureMatchUtilTest : public testing::Test {
 public:
  DirectionalGestureMatchUtilTest() {
    // Pinned since `MaybeMatchDirectionalGesture` reads these; a default
    // change shouldn't silently alter what these tests cover.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"CustomGestureResamplePointCount", "128"},
         {"CustomGestureMaxMatchDistance", "0.14"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(DirectionalGestureMatchUtilTest,
       ShouldReturnMatchWhenStrokesAreMappedAndPathIsCloseToCanonical) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kLeft};

  EXPECT_THAT(MaybeMatchDirectionalGesture(directional_gesture_mapping, strokes,
                                           {{100, 0}, {0, 0}}),
              testing::Optional(testing::Field(
                  &DirectionalGestureMatchInfo::actions,
                  testing::ElementsAre(MakeGestureAction(test::kFakeBack)))));
}

TEST_F(DirectionalGestureMatchUtilTest,
       ShouldReturnNulloptWhenStrokesHaveNoMappedActions) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  const DirectionalGestureStrokes strokes{
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp,
      DirectionalGestureStrokeType::kUp, DirectionalGestureStrokeType::kUp};

  EXPECT_FALSE(MaybeMatchDirectionalGesture(directional_gesture_mapping,
                                            strokes, {{0, 100}, {0, 0}}));
}

TEST_F(DirectionalGestureMatchUtilTest,
       ShouldReturnNulloptWhenDrawnPathIsTooFarFromCanonicalShape) {
  DirectionalGestureMapping directional_gesture_mapping(test::kDefaultCommands);
  const DirectionalGestureStrokes strokes{DirectionalGestureStrokeType::kLeft};

  // Geometrically nothing like a single leftward stroke, so it must fail the
  // match-distance threshold even though `strokes` itself is mapped.
  EXPECT_FALSE(MaybeMatchDirectionalGesture(
      directional_gesture_mapping, strokes, {{0, 0}, {0, 100}, {100, 100}}));
}

}  // namespace gestures
