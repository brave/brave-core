// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/gestures/widget/gesture_view.h"

#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/views/test/views_drawing_test_utils.h"
#include "ui/views/test/views_test_base.h"

namespace gestures {

namespace {

// A point roughly in the middle of a straight trail drawn across the view,
// and one well outside both the trail and the label.
constexpr gfx::Point kOnTrail(100, 100);
constexpr gfx::Point kBlankPoint(10, 10);

SkBitmap PaintGestureView(GestureView& gesture_view) {
  gesture_view.SetSize(gfx::Size(200, 200));
  return views::test::PaintViewToBitmap(&gesture_view);
}

}  // namespace

class GestureViewTest : public views::ViewsTestBase {};

TEST_F(GestureViewTest, ShouldNotProcessEventsWithinSubtree) {
  GestureView gesture_view;

  EXPECT_FALSE(gesture_view.GetCanProcessEventsWithinSubtree());
}

TEST_F(GestureViewTest, ShouldDrawNothingBeforeTrailIsSet) {
  GestureView gesture_view;

  EXPECT_EQ(
      gfx::kPlaceholderColor,
      PaintGestureView(gesture_view).getColor(kOnTrail.x(), kOnTrail.y()));
}

TEST_F(GestureViewTest, ShouldDrawTrailWhenVisible) {
  GestureView gesture_view;
  gesture_view.SetTrail(/*show_trail=*/true,
                        {gfx::PointF(10, 100), gfx::PointF(190, 100)}, u"");

  EXPECT_NE(
      gfx::kPlaceholderColor,
      PaintGestureView(gesture_view).getColor(kOnTrail.x(), kOnTrail.y()));
}

TEST_F(GestureViewTest, ShouldNotDrawTrailWhenHidden) {
  GestureView gesture_view;
  gesture_view.SetTrail(/*show_trail=*/false,
                        {gfx::PointF(10, 100), gfx::PointF(190, 100)}, u"");

  EXPECT_EQ(
      gfx::kPlaceholderColor,
      PaintGestureView(gesture_view).getColor(kOnTrail.x(), kOnTrail.y()));
}

TEST_F(GestureViewTest, ShouldNotDrawTrailWithFewerThanTwoPoints) {
  GestureView gesture_view;
  gesture_view.SetTrail(/*show_trail=*/true, {gfx::PointF(100, 100)}, u"");

  EXPECT_EQ(
      gfx::kPlaceholderColor,
      PaintGestureView(gesture_view).getColor(kOnTrail.x(), kOnTrail.y()));
}

TEST_F(GestureViewTest, ShouldDrawLabelBelowLastPointWhenSet) {
  GestureView gesture_view;
  // The label is centered below this point, so some pixel directly below it
  // must be covered by the label's background.
  gesture_view.SetTrail(/*show_trail=*/false, {gfx::PointF(100, 100)}, u"Back");

  EXPECT_NE(gfx::kPlaceholderColor,
            PaintGestureView(gesture_view).getColor(100, 130));
}

TEST_F(GestureViewTest, ShouldNotDrawEmptyLabel) {
  GestureView gesture_view;
  gesture_view.SetTrail(/*show_trail=*/false, {gfx::PointF(100, 100)}, u"");

  EXPECT_EQ(gfx::kPlaceholderColor,
            PaintGestureView(gesture_view).getColor(100, 130));
}

TEST_F(GestureViewTest, ShouldNotDrawLabelWhenPathIsEmpty) {
  GestureView gesture_view;
  gesture_view.SetTrail(/*show_trail=*/false, {}, u"Back");

  EXPECT_EQ(gfx::kPlaceholderColor,
            PaintGestureView(gesture_view)
                .getColor(kBlankPoint.x(), kBlankPoint.y()));
}

}  // namespace gestures
