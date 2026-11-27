// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/gestures/core/browser/input/touch/touch_centroid_util.h"

#include "brave/components/gestures/core/browser/input/touch/test/web_touch_event_test_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

TEST(TouchCentroidUtilTest, ReturnsOriginForNoTouches) {
  EXPECT_EQ(gfx::PointF(), TouchCentroid(test::MakeWebTouchEvent({})));
}

TEST(TouchCentroidUtilTest, ReturnsPositionOfSingleActiveTouch) {
  const blink::WebTouchEvent web_touch_event =
      test::MakeWebTouchEvent({test::MakeTouchPoint(
          10, 20, blink::WebTouchPoint::State::kStatePressed)});

  EXPECT_EQ(gfx::PointF(10, 20), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, AveragesTwoActiveTouches) {
  const blink::WebTouchEvent web_touch_event = test::MakeWebTouchEvent(
      {test::MakeTouchPoint(0, 0, blink::WebTouchPoint::State::kStatePressed),
       test::MakeTouchPoint(20, 40, blink::WebTouchPoint::State::kStateMoved)});

  EXPECT_EQ(gfx::PointF(10, 20), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, ExcludesTouchesThatHaveAlreadyLifted) {
  const blink::WebTouchEvent web_touch_event = test::MakeWebTouchEvent(
      {test::MakeTouchPoint(0, 0, blink::WebTouchPoint::State::kStateReleased),
       test::MakeTouchPoint(20, 40, blink::WebTouchPoint::State::kStateMoved)});

  EXPECT_EQ(gfx::PointF(20, 40), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, FallsBackToFirstTouchWhenNoneAreActive) {
  const blink::WebTouchEvent web_touch_event =
      test::MakeWebTouchEvent({test::MakeTouchPoint(
          5, 5, blink::WebTouchPoint::State::kStateCancelled)});

  EXPECT_EQ(gfx::PointF(5, 5), TouchCentroid(web_touch_event));
}

}  // namespace gestures
