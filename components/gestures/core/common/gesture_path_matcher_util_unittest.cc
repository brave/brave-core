// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_matcher_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathMatcherUtilTest,
     ShouldReturnZeroDistanceForIdenticalOpenPaths) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};

  EXPECT_FLOAT_EQ(0.0F, BestCandidateGesturePathDistance(
                            gesture_path, gesture_path,
                            /*candidate_gesture_path_is_closed=*/false));
}

TEST(GesturePathMatcherUtilTest,
     ShouldReturnAveragePointwiseDistanceForOpenPaths) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};
  const GesturePath candidate_gesture_path = {gfx::PointF(0, 1),
                                              gfx::PointF(1, 1)};

  EXPECT_FLOAT_EQ(1.0F, BestCandidateGesturePathDistance(
                            gesture_path, candidate_gesture_path,
                            /*candidate_gesture_path_is_closed=*/false));
}

TEST(GesturePathMatcherUtilTest,
     ShouldFindBestAlignmentWhenClosedPathsStartAtDifferentPoints) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                    gfx::PointF(1, 1)};
  // The same 3 points as `gesture_path`, rotated by one position; a closed
  // gesture_path can start from any point along its perimeter.
  const GesturePath candidate_gesture_path = {
      gfx::PointF(1, 0), gfx::PointF(1, 1), gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(0.0F, BestCandidateGesturePathDistance(
                            gesture_path, candidate_gesture_path,
                            /*candidate_gesture_path_is_closed=*/true));
}

TEST(GesturePathMatcherUtilTest,
     ShouldMatchClosedPathsDrawnInOppositeDirections) {
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                    gfx::PointF(1, 1)};
  // The same triangle, traced in the opposite winding direction; a closed
  // shape has no meaningful winding direction, so it should still match.
  const GesturePath candidate_gesture_path = {
      gfx::PointF(1, 1), gfx::PointF(1, 0), gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(0.0F, BestCandidateGesturePathDistance(
                            gesture_path, candidate_gesture_path,
                            /*candidate_gesture_path_is_closed=*/true));
}

TEST(GesturePathMatcherUtilTest,
     ShouldReturnLargeDistanceForDissimilarClosedShapes) {
  // No rotation of `candidate_gesture_path` lines up with `gesture_path`,
  // confirming the every-rotation search doesn't just always collapse to zero.
  const GesturePath gesture_path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                    gfx::PointF(1, 1)};
  const GesturePath candidate_gesture_path = {
      gfx::PointF(0, 0), gfx::PointF(0, 1), gfx::PointF(1, 0)};

  EXPECT_GT(BestCandidateGesturePathDistance(
                gesture_path, candidate_gesture_path,
                /*candidate_gesture_path_is_closed=*/true),
            0.1F);
}

}  // namespace gestures
