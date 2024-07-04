/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/bounds_util.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

namespace {

constexpr gfx::Point kWorkAreaOrigin(20, 10);
constexpr gfx::Size kWorkAreaSize(2000, 1000);
constexpr gfx::Size kBoundsSize(100, 50);
constexpr int kShiftX = 300;
constexpr int kShiftY = 200;

}  // namespace

TEST(BraveAdsBoundsUtilTest, CheckSnapBoundsToEdgeOfWorkArea) {
  const gfx::Rect work_area(kWorkAreaOrigin, kWorkAreaSize);
  gfx::Rect bounds(kBoundsSize);

  // Position near the left edge.
  gfx::Point initial_origin(work_area.x() + kShiftX,
                            work_area.y() + work_area.height() / 2);
  bounds.set_origin(initial_origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x(), bounds.x());
  EXPECT_EQ(initial_origin.y(), bounds.y());

  // Position near the right edge.
  initial_origin = gfx::Point(work_area.width() - kShiftX,
                              work_area.y() + work_area.height() / 2);
  bounds.set_origin(initial_origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x() + work_area.width() - bounds.width(), bounds.x());
  EXPECT_EQ(initial_origin.y(), bounds.y());

  // Position near the top edge.
  initial_origin = gfx::Point(work_area.x() + work_area.height() / 2,
                              work_area.y() + kShiftY);
  bounds.set_origin(initial_origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(initial_origin.x(), bounds.x());
  EXPECT_EQ(work_area.y(), bounds.y());

  // Position near the bottom edge.
  initial_origin = gfx::Point(work_area.x() + work_area.height() / 2,
                              work_area.height() - kShiftY);
  bounds.set_origin(initial_origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(initial_origin.x(), bounds.x());
  EXPECT_EQ(work_area.y() + work_area.height() - bounds.height(), bounds.y());
}

}  // namespace brave_ads
