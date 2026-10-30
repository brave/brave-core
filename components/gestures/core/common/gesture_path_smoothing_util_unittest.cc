// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/common/gesture_path_smoothing_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(GesturePathSmoothingUtilTest, StartsAtTheFirstPoint) {
  const GesturePath points = {gfx::PointF(0, 0), gfx::PointF(10, 20),
                              gfx::PointF(30, 10)};

  const SkPath gesture_path = SmoothedPath(points);

  EXPECT_EQ(SkPoint::Make(0, 0), gesture_path.getPoint(0));
}

TEST(GesturePathSmoothingUtilTest, EndsAtTheLastPoint) {
  const GesturePath points = {gfx::PointF(0, 0), gfx::PointF(10, 20),
                              gfx::PointF(30, 10)};

  const SkPath gesture_path = SmoothedPath(points);

  EXPECT_EQ(SkPoint::Make(30, 10), *gesture_path.getLastPt());
}

TEST(GesturePathSmoothingUtilTest, TwoPointsProduceAStraightLine) {
  const GesturePath points = {gfx::PointF(0, 0), gfx::PointF(10, 10)};

  const SkPath gesture_path = SmoothedPath(points);

  EXPECT_EQ(SkPoint::Make(0, 0), gesture_path.getPoint(0));
  EXPECT_EQ(SkPoint::Make(10, 10), *gesture_path.getLastPt());
}

TEST(GesturePathSmoothingUtilTest, CurvesThroughABend) {
  const GesturePath points = {gfx::PointF(0, 0), gfx::PointF(50, 0),
                              gfx::PointF(50, 50)};

  const SkPath gesture_path = SmoothedPath(points);

  // 3+ points are joined by Catmull-Rom cubic Bezier segments, not straight
  // lines, so the curve bends through the point rather than tracing it.
  bool has_cubic_segment = false;
  SkPath::Iter iter(gesture_path, /*forceClose=*/false);
  SkPoint iter_points[4];
  for (SkPath::Verb verb = iter.next(iter_points); verb != SkPath::kDone_Verb;
       verb = iter.next(iter_points)) {
    if (verb == SkPath::kCubic_Verb) {
      has_cubic_segment = true;
      break;
    }
  }
  EXPECT_TRUE(has_cubic_segment);
}

TEST(GesturePathSmoothingUtilTest, BoundsContainEveryPoint) {
  const GesturePath points = {gfx::PointF(0, 50), gfx::PointF(100, 0),
                              gfx::PointF(50, 100)};

  const SkPath gesture_path = SmoothedPath(points);

  // `SkRect::contains` excludes the right/bottom edge, which a gesture_path
  // vertex can legitimately sit on, so bounds are compared inclusively instead.
  const SkRect& bounds = gesture_path.getBounds();
  for (const gfx::PointF& point : points) {
    EXPECT_GE(point.x(), bounds.left());
    EXPECT_LE(point.x(), bounds.right());
    EXPECT_GE(point.y(), bounds.top());
    EXPECT_LE(point.y(), bounds.bottom());
  }
}

}  // namespace gestures
