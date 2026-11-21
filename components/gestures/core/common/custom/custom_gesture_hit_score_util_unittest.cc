// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_hit_score_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(CustomGestureHitScoreUtilTest, HitScoreShouldReturnOneForIdenticalPaths) {
  EXPECT_FLOAT_EQ(
      1.0F, HitScore(/*gesture_path=*/{gfx::PointF(0, 0), gfx::PointF(1, 0)},
                     /*candidate_gesture_path=*/
                     {gfx::PointF(0, 0), gfx::PointF(1, 0)},
                     /*per_point_threshold=*/1));
}

TEST(CustomGestureHitScoreUtilTest,
     HitScoreShouldReturnZeroWhenEveryPointExceedsThreshold) {
  EXPECT_FLOAT_EQ(
      0.0F, HitScore(/*gesture_path=*/{gfx::PointF(0, 0), gfx::PointF(1, 0)},
                     /*candidate_gesture_path=*/
                     {gfx::PointF(0, 10), gfx::PointF(1, 10)},
                     /*per_point_threshold=*/1));
}

TEST(CustomGestureHitScoreUtilTest,
     HitScoreShouldFallOffLinearlyWithinThreshold) {
  EXPECT_FLOAT_EQ(0.5F,
                  HitScore(/*gesture_path=*/{gfx::PointF(0, 0)},
                           /*candidate_gesture_path=*/{gfx::PointF(0.5, 0)},
                           /*per_point_threshold=*/1));
}

TEST(CustomGestureHitScoreUtilTest, HitScoreShouldReturnZeroForEmptyPath) {
  EXPECT_FLOAT_EQ(0.0F, HitScore(/*gesture_path=*/{},
                                 /*candidate_gesture_path=*/{gfx::PointF(0, 0)},
                                 /*per_point_threshold=*/1));
}

TEST(CustomGestureHitScoreUtilTest,
     HitScoreShouldReturnZeroForNonPositiveThreshold) {
  EXPECT_FLOAT_EQ(0.0F, HitScore(/*gesture_path=*/{gfx::PointF(0, 0)},
                                 /*candidate_gesture_path=*/{gfx::PointF(0, 0)},
                                 /*per_point_threshold=*/0));
}

TEST(CustomGestureHitScoreUtilTest,
     BestHitScoreShouldMatchHitScoreForOpenCandidates) {
  EXPECT_FLOAT_EQ(HitScore(
                      /*gesture_path=*/{gfx::PointF(0, 0), gfx::PointF(1, 0)},
                      /*candidate_gesture_path=*/
                      {gfx::PointF(0, 1), gfx::PointF(1, 1)},
                      /*per_point_threshold=*/1),
                  BestHitScore(
                      /*gesture_path=*/{gfx::PointF(0, 0), gfx::PointF(1, 0)},
                      /*candidate_gesture_path=*/
                      {gfx::PointF(0, 1), gfx::PointF(1, 1)},
                      /*candidate_gesture_path_is_closed=*/false,
                      /*per_point_threshold=*/1));
}

TEST(CustomGestureHitScoreUtilTest,
     BestHitScoreShouldFindBestRotationForClosedCandidates) {
  EXPECT_FLOAT_EQ(1.0F,
                  BestHitScore(
                      /*gesture_path=*/{gfx::PointF(0, 0), gfx::PointF(1, 0),
                                        gfx::PointF(1, 1)},
                      /*candidate_gesture_path=*/
                      {gfx::PointF(1, 0), gfx::PointF(1, 1), gfx::PointF(0, 0)},
                      /*candidate_gesture_path_is_closed=*/true,
                      /*per_point_threshold=*/1));
}

}  // namespace gestures
