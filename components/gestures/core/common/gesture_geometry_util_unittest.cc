// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_geometry_util.h"

#include "base/test/scoped_feature_list.h"
#include "brave/components/gestures/core/common/features.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

// Mirrors `GestureDrawSession::ClassifyResampledDirectionalStrokes`'s real
// `min_stroke_pixels / 3.0` resample-interval derivation, so these path
// shapes are tested at a realistic resampling granularity. A plain literal
// rather than read from the feature, so a future default change does not
// silently change what these tests exercise.
constexpr double kMinReversalDistance = 10.0;

}  // namespace

TEST(GesturePathLengthTest, SumsConsecutiveSegmentDistances) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(30, 0),
                            gfx::PointF(30, 40)};

  EXPECT_FLOAT_EQ(GesturePathLength(path), 70.0F);
}

TEST(GesturePathLengthTest, DoesNotShrinkAsAPathCurvesBackNearItsStart) {
  // Net start-to-end displacement here is small, but the path traveled a
  // long way to get there, which is exactly what this is meant to capture
  // (e.g. for `IsWithinSmartGestureMaxDistance` in `GestureDrawSession`).
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(100, 0),
                            gfx::PointF(100, 100), gfx::PointF(5, 100)};

  EXPECT_FLOAT_EQ(GesturePathLength(path), 295.0F);
}

TEST(RetainSignificantGeometryTest, PreservesAStraightLineAsItsEndpoints) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(50, 0),
                            gfx::PointF(100, 0), gfx::PointF(150, 0)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(150, 0)));
}

TEST(RetainSignificantGeometryTest, DoesNotCreateATurnFromStationaryJitter) {
  // Bounces between two points 1px apart, well under
  // `kMinReversalDistance`.
  const GesturePath path = {gfx::PointF(100, 0), gfx::PointF(99, 0),
                            gfx::PointF(100, 0), gfx::PointF(99, 0),
                            gfx::PointF(100, 0), gfx::PointF(99, 0)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(100, 0), gfx::PointF(99, 0)));
}

TEST(RetainSignificantGeometryTest,
     PreservesAShortExcursionBeforeALongerReversal) {
  // A short leftward dip (bottoming out at x=78, a 22px excursion from the
  // start) followed by a curve and a much longer rightward run. The
  // leftward dip never travels far enough on its own to commit as a
  // directional stroke, but its extremum must still survive here.
  const GesturePath path = {
      gfx::PointF(100, 0), gfx::PointF(95, 0), gfx::PointF(88, 0),
      gfx::PointF(80, 0),  gfx::PointF(78, 0),  // Leftmost extremum.
      gfx::PointF(81, 0),  gfx::PointF(87, 0), gfx::PointF(96, 0),
      gfx::PointF(110, 0), gfx::PointF(130, 0)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(100, 0), gfx::PointF(78, 0),
                                   gfx::PointF(130, 0)));
}

TEST(RetainSignificantGeometryTest, PreservesATightHookShape) {
  // A short rightward run that sharply reverses into a short downward hook,
  // each leg well under a full directional stroke's length but each a
  // clear, deliberate reversal relative to `kMinReversalDistance`.
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(15, 0),
                            gfx::PointF(25, 0),  // Rightmost extremum.
                            gfx::PointF(25, 15), gfx::PointF(25, 25)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(25, 0),
                                   gfx::PointF(25, 25)));
}

TEST(RetainSignificantGeometryTest,
     PreservesAWobblySwipeAsStartAndEndWhenWobbleIsSmall) {
  // Wanders a few pixels off a straight rightward path, well under
  // `kMinReversalDistance`, the way a real hand-drawn "straight" swipe does.
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(50, 3),
                            gfx::PointF(100, -2), gfx::PointF(150, 2),
                            gfx::PointF(200, 0)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(200, 0)));
}

TEST(RetainSignificantGeometryTest,
     DoesNotCreateATurnFromATinyTremorDuringALongerSwipe) {
  // A small backward tremor partway through an otherwise-long rightward
  // swipe; the tremor's own reversal is well under `kMinReversalDistance`,
  // so it must not register as a meaningful extremum of its own.
  const GesturePath path = {
      gfx::PointF(0, 0),   gfx::PointF(50, 0),
      gfx::PointF(55, 0),  gfx::PointF(52, 0),  // Tiny backward tremor.
      gfx::PointF(100, 0), gfx::PointF(150, 0)};

  EXPECT_THAT(RetainSignificantGeometry(path, kMinReversalDistance),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(150, 0)));
}

TEST(GestureGeometryUtilTest, ShouldReturnRequestedPointCountAfterResampling) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 5), testing::SizeIs(5));
}

TEST(GestureGeometryUtilTest,
     ShouldEvenlySpaceAndScaleStraightLineWhenNormalizing) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 3),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(0.5, 0),
                                   gfx::PointF(1, 0)));
}

TEST(GestureGeometryUtilTest, ShouldResamplePointsEvenlyFromSingleSegment) {
  // A 2-point input resampled to many more output points requires repeated
  // sub-division of that single original segment. Each extracted point must
  // advance from the previous one, not re-interpolate from the segment's
  // fixed original start every time, or points bunch up near the start
  // instead of spacing evenly (caught at 3 output points, the smallest
  // case, only by coincidence, since the last of 2 extractions always
  // lands exactly on the segment's end regardless of which anchor is used).
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 5),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(0.25, 0),
                                   gfx::PointF(0.5, 0), gfx::PointF(0.75, 0),
                                   gfx::PointF(1, 0)));
}

TEST(GestureGeometryUtilTest, ShouldCollapseZeroLengthPathToOrigin) {
  const GesturePath path = {gfx::PointF(5, 5), gfx::PointF(5, 5)};

  EXPECT_THAT(ResampleAndNormalizePath(path, 4),
              testing::ElementsAre(gfx::PointF(0, 0), gfx::PointF(0, 0),
                                   gfx::PointF(0, 0), gfx::PointF(0, 0)));
}

class IsClosedPathTest : public testing::Test {
 public:
  IsClosedPathTest() {
    scoped_feature_list_.InitAndEnableFeatureWithParameters(
        features::kGesturesFeature,
        {{"CustomGestureClosedPathGapRatio", "0.2"}});
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

TEST_F(IsClosedPathTest, ReturnsFalseForZeroLengthPath) {
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(0, 0)};

  EXPECT_FALSE(IsClosedPath(raw_points));
}

TEST_F(IsClosedPathTest, ReturnsFalseWhenTheEndIsFarFromTheStart) {
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(10, 0)};

  EXPECT_FALSE(IsClosedPath(raw_points));
}

TEST_F(IsClosedPathTest, ReturnsTrueWhenTheEndIsCloseToTheStart) {
  // Length 19.5, gap 0.5, a ratio of ~0.026, comfortably under the pinned
  // 0.2 threshold.
  const GesturePath raw_points = {gfx::PointF(0, 0), gfx::PointF(10, 0),
                                  gfx::PointF(0.5, 0)};

  EXPECT_TRUE(IsClosedPath(raw_points));
}

TEST(GestureGeometryUtilTest, ShouldReturnZeroDistanceForIdenticalOpenPaths) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};

  EXPECT_FLOAT_EQ(
      0.0F, DistanceToCandidate(path, path, /*candidate_is_closed=*/false));
}

TEST(GestureGeometryUtilTest, ShouldReturnAverageOffsetForOpenPaths) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(1, 0)};
  const GesturePath candidate_path = {gfx::PointF(0, 1), gfx::PointF(1, 1)};

  EXPECT_FLOAT_EQ(1.0F, DistanceToCandidate(path, candidate_path,
                                            /*candidate_is_closed=*/false));
}

TEST(GestureGeometryUtilTest,
     ShouldFindBestAlignmentWhenClosedPathsStartAtDifferentPoints) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                            gfx::PointF(1, 1)};
  // The same 3 points as `path`, but rotated by one position, so comparing
  // point-by-point without rotating would show a large distance, even
  // though the shapes are identical once a closed path is allowed to
  // start from any point along its perimeter.
  const GesturePath candidate_path = {gfx::PointF(1, 0), gfx::PointF(1, 1),
                                      gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(0.0F, DistanceToCandidate(path, candidate_path,
                                            /*candidate_is_closed=*/true));
}

TEST(GestureGeometryUtilTest, ShouldMatchClosedPathsDrawnInOppositeDirections) {
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                            gfx::PointF(1, 1)};
  // The same triangle as `path`, but with its points in reverse order, as if
  // traced in the opposite (e.g. clockwise vs. counter-clockwise) direction.
  // A closed shape has no meaningful winding direction, so this should match
  // just as well as any rotation of the same direction does.
  const GesturePath candidate_path = {gfx::PointF(1, 1), gfx::PointF(1, 0),
                                      gfx::PointF(0, 0)};

  EXPECT_FLOAT_EQ(0.0F, DistanceToCandidate(path, candidate_path,
                                            /*candidate_is_closed=*/true));
}

TEST(GestureGeometryUtilTest,
     ShouldReturnLargeDistanceForDissimilarClosedShapes) {
  // No rotation of `candidate_path` lines up with `path`, confirming the
  // every-rotation search doesn't just always collapse to zero.
  const GesturePath path = {gfx::PointF(0, 0), gfx::PointF(1, 0),
                            gfx::PointF(1, 1)};
  const GesturePath candidate_path = {gfx::PointF(0, 0), gfx::PointF(0, 1),
                                      gfx::PointF(1, 0)};

  EXPECT_GT(
      DistanceToCandidate(path, candidate_path, /*candidate_is_closed=*/true),
      0.1F);
}

}  // namespace gestures
