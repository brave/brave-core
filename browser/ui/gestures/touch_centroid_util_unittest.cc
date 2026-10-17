// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/touch_centroid_util.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace gestures {

namespace {

blink::WebTouchPoint MakeTouchPoint(float x,
                                    float y,
                                    blink::WebTouchPoint::State state) {
  blink::WebTouchPoint touch_point;
  touch_point.SetPositionInWidget(x, y);
  touch_point.state = state;
  return touch_point;
}

}  // namespace

TEST(TouchCentroidUtilTest, ReturnsOriginForNoTouches) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = 0;

  EXPECT_EQ(gfx::PointF(), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, ReturnsThePositionOfASingleActiveTouch) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = 1;
  web_touch_event.touches[0] =
      MakeTouchPoint(10, 20, blink::WebTouchPoint::State::kStatePressed);

  EXPECT_EQ(gfx::PointF(10, 20), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, AveragesTwoActiveTouches) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = 2;
  web_touch_event.touches[0] =
      MakeTouchPoint(0, 0, blink::WebTouchPoint::State::kStatePressed);
  web_touch_event.touches[1] =
      MakeTouchPoint(20, 40, blink::WebTouchPoint::State::kStateMoved);

  EXPECT_EQ(gfx::PointF(10, 20), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, ExcludesTouchesThatHaveAlreadyLifted) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = 2;
  web_touch_event.touches[0] =
      MakeTouchPoint(0, 0, blink::WebTouchPoint::State::kStateReleased);
  web_touch_event.touches[1] =
      MakeTouchPoint(20, 40, blink::WebTouchPoint::State::kStateMoved);

  EXPECT_EQ(gfx::PointF(20, 40), TouchCentroid(web_touch_event));
}

TEST(TouchCentroidUtilTest, FallsBackToFirstTouchWhenNoneAreActive) {
  blink::WebTouchEvent web_touch_event;
  web_touch_event.touches_length = 1;
  web_touch_event.touches[0] =
      MakeTouchPoint(5, 5, blink::WebTouchPoint::State::kStateCancelled);

  EXPECT_EQ(gfx::PointF(5, 5), TouchCentroid(web_touch_event));
}

}  // namespace gestures
