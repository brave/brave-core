// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_candidate_scoring_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/custom/test/custom_gesture_test_util.h"
#include "brave/components/gestures/core/common/features.h"
#include "brave/components/gestures/core/common/test/gesture_path_test_util.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

class CustomGestureCandidateScoringUtilTest : public testing::Test {
 public:
  CustomGestureCandidateScoringUtilTest() {
    // Pinned since `ScoreCustomGestures` reads these; a default change
    // shouldn't silently alter what these tests cover.
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"CustomGestureResamplePointCount", "128"},
         {"CustomGestureInlierPointThreshold", "0.1"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(CustomGestureCandidateScoringUtilTest,
       ReturnsNoCandidatesForPathBelowMinimumLength) {
  const CustomGestures custom_gestures = {test::MakeCustomGesture(
      /*id=*/"circle", test::MakeCircleGesturePath(), /*actions=*/{})};

  EXPECT_THAT(ScoreCustomGestures(custom_gestures, {gfx::PointF(0, 0)}),
              testing::IsEmpty());
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ReturnsNoCandidatesWhenNoneAreRecorded) {
  EXPECT_THAT(ScoreCustomGestures({}, test::MakeCircleGesturePath()),
              testing::IsEmpty());
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ScoresOneCandidatePerRecordedGesture) {
  const CustomGestures custom_gestures = {
      test::MakeCustomGesture(/*id=*/"circle", test::MakeCircleGesturePath(),
                              /*actions=*/{}),
      test::MakeCustomGesture(/*id=*/"zigzag", test::MakeZigzagGesturePath(),
                              /*actions=*/{})};

  EXPECT_THAT(
      ScoreCustomGestures(custom_gestures, test::MakeCircleGesturePath()),
      testing::SizeIs(2U));
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ScoresIdenticalDrawnAndRecordedPathsAsExactMatch) {
  const CustomGestures custom_gestures = {test::MakeCustomGesture(
      /*id=*/"circle", test::MakeCircleGesturePath(), /*actions=*/{})};

  EXPECT_THAT(
      ScoreCustomGestures(custom_gestures, test::MakeCircleGesturePath()),
      testing::ElementsAre(testing::FieldsAre(
          /*gesture=*/testing::Pointee(
              testing::Field(&CustomGestureInfo::id, "circle")),
          /*hit_score=*/testing::FloatEq(1.0F),
          /*distance=*/testing::FloatEq(0.0F), /*is_closed=*/true)));
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ReturnsNullWhenNoCandidatesQualify) {
  EXPECT_FALSE(BestCustomGestureCandidateWithin({}, /*max_distance=*/1,
                                                /*min_hit_score=*/0.0F));
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ReturnsNullWhenEveryCandidateExceedsMaxDistance) {
  const CustomGestureInfo custom_gesture = test::MakeCustomGesture(
      /*id=*/"circle", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureCandidates gesture_candidates = {
      {&custom_gesture, /*hit_score=*/0.9F, /*distance=*/5.0F,
       /*is_closed=*/true}};

  EXPECT_FALSE(BestCustomGestureCandidateWithin(gesture_candidates,
                                                /*max_distance=*/1.0F,
                                                /*min_hit_score=*/0.0F));
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       ReturnsNullWhenEveryCandidateIsBelowMinHitScore) {
  const CustomGestureInfo custom_gesture = test::MakeCustomGesture(
      /*id=*/"circle", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureCandidates gesture_candidates = {
      {&custom_gesture, /*hit_score=*/0.3F, /*distance=*/0.1F,
       /*is_closed=*/true}};

  EXPECT_FALSE(BestCustomGestureCandidateWithin(gesture_candidates,
                                                /*max_distance=*/1.0F,
                                                /*min_hit_score=*/0.5F));
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       PrefersHigherHitScoreEvenWithGreaterDistance) {
  const CustomGestureInfo weaker_fit = test::MakeCustomGesture(
      /*id=*/"weaker", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureInfo stronger_fit = test::MakeCustomGesture(
      /*id=*/"stronger", test::MakeZigzagGesturePath(), /*actions=*/{});
  const CustomGestureCandidates gesture_candidates = {
      {&weaker_fit, /*hit_score=*/0.5F, /*distance=*/0.1F,
       /*is_closed=*/true},
      {&stronger_fit, /*hit_score=*/0.9F, /*distance=*/0.4F,
       /*is_closed=*/false}};

  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(gesture_candidates,
                                       /*max_distance=*/1.0F,
                                       /*min_hit_score=*/0.0F);

  ASSERT_TRUE(best_gesture_candidate);
  EXPECT_EQ("stronger", best_gesture_candidate->gesture->id);
}

TEST_F(CustomGestureCandidateScoringUtilTest,
       BreaksHitScoreTiesUsingLowerDistance) {
  const CustomGestureInfo farther = test::MakeCustomGesture(
      /*id=*/"farther", test::MakeCircleGesturePath(), /*actions=*/{});
  const CustomGestureInfo closer = test::MakeCustomGesture(
      /*id=*/"closer", test::MakeZigzagGesturePath(), /*actions=*/{});
  const CustomGestureCandidates gesture_candidates = {
      {&farther, /*hit_score=*/0.7F, /*distance=*/0.5F, /*is_closed=*/true},
      {&closer, /*hit_score=*/0.7F, /*distance=*/0.2F, /*is_closed=*/false}};

  const CustomGestureCandidateInfo* best_gesture_candidate =
      BestCustomGestureCandidateWithin(gesture_candidates,
                                       /*max_distance=*/1.0F,
                                       /*min_hit_score=*/0.0F);

  ASSERT_TRUE(best_gesture_candidate);
  EXPECT_EQ("closer", best_gesture_candidate->gesture->id);
}

}  // namespace gestures
