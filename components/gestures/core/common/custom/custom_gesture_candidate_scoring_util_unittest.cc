// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_scoring_util.h"

#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(CustomGestureCandidateScoringUtilTest,
     ReturnsNoCandidatesForATooShortPath) {
  const CustomGestures custom_gestures = {test::MakeCustomGesture(
      "circle", test::MakeCircleGesturePath(), /*actions=*/{})};

  EXPECT_THAT(CustomGestureCandidates(custom_gestures, {gfx::PointF(0, 0)}),
              testing::IsEmpty());
}

TEST(CustomGestureCandidateScoringUtilTest,
     ReturnsNoCandidatesWhenNoneAreRecorded) {
  EXPECT_THAT(CustomGestureCandidates({}, test::MakeCircleGesturePath()),
              testing::IsEmpty());
}

TEST(CustomGestureCandidateScoringUtilTest,
     ScoresOneCandidatePerRecordedGesture) {
  const CustomGestures custom_gestures = {
      test::MakeCustomGesture("circle", test::MakeCircleGesturePath(),
                              /*actions=*/{}),
      test::MakeCustomGesture("zigzag", test::MakeZigzagGesturePath(),
                              /*actions=*/{})};

  EXPECT_THAT(
      CustomGestureCandidates(custom_gestures, test::MakeCircleGesturePath()),
      testing::SizeIs(2U));
}

TEST(CustomGestureCandidateScoringUtilTest,
     ScoresTheDrawnPathAsAnExactMatchForItself) {
  const CustomGestures custom_gestures = {test::MakeCustomGesture(
      "circle", test::MakeCircleGesturePath(), /*actions=*/{})};

  EXPECT_THAT(
      CustomGestureCandidates(custom_gestures, test::MakeCircleGesturePath()),
      testing::ElementsAre(testing::FieldsAre(
          /*custom_gesture=*/testing::Pointee(
              testing::Field(&CustomGestureInfo::id, "circle")),
          /*hit_score=*/testing::FloatEq(1.0F),
          /*distance=*/testing::FloatEq(0.0F), /*is_closed=*/true)));
}

TEST(CustomGestureCandidateScoringUtilTest, ReturnsNullWhenNoCandidatesFit) {
  EXPECT_FALSE(BestCustomGestureCandidateWithin({}, /*max_distance=*/1));
}

TEST(CustomGestureCandidateScoringUtilTest,
     ReturnsNullWhenEveryCandidateExceedsMaxDistance) {
  const CustomGestureInfo custom_gesture = test::MakeCustomGesture(
      "circle", test::MakeCircleGesturePath(), /*actions=*/{});
  const std::vector<CustomGestureCandidateInfo> gesture_candidates = {
      {&custom_gesture, /*hit_score=*/0.9F, /*distance=*/5.0F,
       /*is_closed=*/true}};

  EXPECT_FALSE(BestCustomGestureCandidateWithin(gesture_candidates,
                                                /*max_distance=*/1.0F));
}

TEST(CustomGestureCandidateScoringUtilTest,
     PrefersHigherHitScoreEvenWithGreaterDistance) {
  const CustomGestureInfo weaker_fit = test::MakeCustomGesture(
      "weaker", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureInfo stronger_fit = test::MakeCustomGesture(
      "stronger", test::MakeZigzagGesturePath(), /*actions=*/{});
  const std::vector<CustomGestureCandidateInfo> gesture_candidates = {
      {&weaker_fit, /*hit_score=*/0.5F, /*distance=*/0.1F,
       /*is_closed=*/true},
      {&stronger_fit, /*hit_score=*/0.9F, /*distance=*/0.4F,
       /*is_closed=*/false}};

  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(gesture_candidates,
                                       /*max_distance=*/1.0F);

  ASSERT_TRUE(best_gesture_candidate);
  EXPECT_EQ("stronger", best_gesture_candidate->custom_gesture->id);
}

TEST(CustomGestureCandidateScoringUtilTest,
     BreaksHitScoreTiesUsingLowerDistance) {
  const CustomGestureInfo farther = test::MakeCustomGesture(
      "farther", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureInfo closer = test::MakeCustomGesture(
      "closer", test::MakeZigzagGesturePath(), /*actions=*/{});
  const std::vector<CustomGestureCandidateInfo> gesture_candidates = {
      {&farther, /*hit_score=*/0.7F, /*distance=*/0.5F, /*is_closed=*/true},
      {&closer, /*hit_score=*/0.7F, /*distance=*/0.2F, /*is_closed=*/false}};

  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(gesture_candidates,
                                       /*max_distance=*/1.0F);

  ASSERT_TRUE(best_gesture_candidate);
  EXPECT_EQ("closer", best_gesture_candidate->custom_gesture->id);
}

}  // namespace gestures
