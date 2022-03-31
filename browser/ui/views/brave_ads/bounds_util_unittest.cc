// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/brave_ads/bounds_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

// npm run test -- brave_unit_tests --filter=BoundsUtilTest.*

namespace {

constexpr gfx::Point work_area_origin(20, 10);
constexpr gfx::Size work_area_size(2000, 1000);
constexpr gfx::Size bounds_size(100, 50);
constexpr int shift_x = 300;
constexpr int shift_y = 200;

}  // namespace

TEST(BoundsUtilTest, CheckSnapBoundsToEdgeOfWorkArea) {
  const gfx::Rect work_area(work_area_origin, work_area_size);
  gfx::Rect bounds(bounds_size);

  // Position near the left edge.
  gfx::Point initial_origin(work_area.x() + shift_x,
                            work_area.y() + work_area.height() / 2);
  bounds.set_origin(initial_origin);
  brave_ads::SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x(), bounds.x());
  EXPECT_EQ(initial_origin.y(), bounds.y());

  // Position near the right edge.
  initial_origin = gfx::Point(work_area.width() - shift_x,
                              work_area.y() + work_area.height() / 2);
  bounds.set_origin(initial_origin);
  brave_ads::SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x() + work_area.width() - bounds.width(), bounds.x());
  EXPECT_EQ(initial_origin.y(), bounds.y());

  // Position near the top edge.
  initial_origin = gfx::Point(work_area.x() + work_area.height() / 2,
                              work_area.y() + shift_y);
  bounds.set_origin(initial_origin);
  brave_ads::SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(initial_origin.x(), bounds.x());
  EXPECT_EQ(work_area.y(), bounds.y());

  // Position near the bottom edge.
  initial_origin = gfx::Point(work_area.x() + work_area.height() / 2,
                              work_area.height() - shift_y);
  bounds.set_origin(initial_origin);
  brave_ads::SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(initial_origin.x(), bounds.x());
  EXPECT_EQ(work_area.y() + work_area.height() - bounds.height(), bounds.y());
}
