// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/custom/custom_gesture_geometry_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(CustomGestureGeometryUtilTest,
     ResampleAndNormalizePathReturnsExactlyPointCount) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 5), testing::SizeIs(5));
}

TEST(CustomGestureGeometryUtilTest,
     ResampleAndNormalizePathEvenlySpacesAndScalesAStraightLine) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 3),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(0.5, 0),
                                   gfx::PointF(1, 0)));
}

TEST(CustomGestureGeometryUtilTest,
     ResampleAndNormalizePathCollapsesAZeroLengthPathToTheOrigin) {
  const std::vector<gfx::PointF> path = {gfx::PointF(5, 5), gfx::PointF(5, 5)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 4),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(0, 0),
                                   gfx::PointF(0, 0), gfx::PointF(0, 0)));
}

class IsClosedPathTest : public testing::Test {
 public:
  IsClosedPathTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature, {{"ClosedPathGapRatio", "0.2"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(IsClosedPathTest, ReturnsFalseForZeroLengthPath) {
  const std::vector<gfx::PointF> raw_points = {gfx::PointF(0, 0),
                                               gfx::PointF(0, 0)};

  EXPECT_FALSE(IsClosedPath(raw_points));
}

TEST_F(IsClosedPathTest, ReturnsFalseWhenTheEndIsFarFromTheStart) {
  const std::vector<gfx::PointF> raw_points = {gfx::PointF(0, 0),
                                               gfx::PointF(10, 0)};

  EXPECT_FALSE(IsClosedPath(raw_points));
}

TEST_F(IsClosedPathTest, ReturnsTrueWhenTheEndIsCloseToTheStart) {
  // Length 19.5, gap 0.5: a ratio of ~0.026, comfortably under the pinned
  // 0.2 threshold.
  const std::vector<gfx::PointF> raw_points = {
      gfx::PointF(0, 0), gfx::PointF(10, 0), gfx::PointF(0.5, 0)};

  EXPECT_TRUE(IsClosedPath(raw_points));
}

TEST(CustomGestureGeometryUtilTest,
     DistanceToCandidateReturnsZeroForIdenticalOpenPaths) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};

  EXPECT_FLOAT_EQ(
      0.0F, DistanceToCandidate(path, path, /*candidate_is_closed=*/false));
}

TEST(CustomGestureGeometryUtilTest,
     DistanceToCandidateReturnsTheAverageOffsetForOpenPaths) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};
  const std::vector<gfx::PointF> candidate = {gfx::PointF(0, 1),
                                              gfx::PointF(1, 1)};

  EXPECT_FLOAT_EQ(1.0F, DistanceToCandidate(path, candidate,
                                            /*candidate_is_closed=*/false));
}

TEST(CustomGestureGeometryUtilTest,
     DistanceToCandidateTriesEveryRotationForClosedCandidate) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                         gfx::PointF(1, 1)};
  // The same 3 points as `path`, but rotated by one position: comparing
  // point-by-point without rotating would show a large distance, even
  // though the shapes are identical once a closed path is allowed to
  // start from any point along its perimeter.
  const std::vector<gfx::PointF> candidate = {
      gfx::PointF(1, 0), gfx::PointF(1, 1), gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(
      0.0F, DistanceToCandidate(path, candidate, /*candidate_is_closed=*/true));
}

TEST(CustomGestureGeometryUtilTest,
     DistanceToCandidateMatchesClosedCandidateDrawnInTheOppositeDirection) {
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                         gfx::PointF(1, 1)};
  // The same triangle as `path`, but with its points in reverse order, as if
  // traced in the opposite (e.g. clockwise vs. counter-clockwise) direction.
  // A closed shape has no meaningful winding direction, so this should match
  // just as well as any rotation of the same direction does.
  const std::vector<gfx::PointF> candidate = {
      gfx::PointF(1, 1), gfx::PointF(1, 0), gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(
      0.0F, DistanceToCandidate(path, candidate, /*candidate_is_closed=*/true));
}

TEST(CustomGestureGeometryUtilTest,
     DistanceToCandidateReturnsLargeDistanceForClosedShapesThatDoNotMatch) {
  // No rotation of `candidate` lines up with `path`: confirms the
  // every-rotation search above doesn't just always collapse to zero.
  const std::vector<gfx::PointF> path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                                         gfx::PointF(1, 1)};
  const std::vector<gfx::PointF> candidate = {
      gfx::PointF(0, 0), gfx::PointF(0, 1), gfx::PointF(1, 0)};

  EXPECT_GT(DistanceToCandidate(path, candidate, /*candidate_is_closed=*/true),
            0.1F);
}

}  // namespace gestures
