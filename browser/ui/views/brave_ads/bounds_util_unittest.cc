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
constexpr int kWorkAreaOffsetX = 300;
constexpr int kWorkAreaOffsetY = 200;

constexpr gfx::Size kBoundsSize(100, 50);

}  // namespace

TEST(BraveAdsBoundsUtilTest, SnapBoundsToEdgeOfWorkArea) {
  const gfx::Rect work_area(kWorkAreaOrigin, kWorkAreaSize);

  gfx::Rect bounds(kBoundsSize);

  gfx::Point origin;

  // Position near the left edge.
  origin = {/*x=*/work_area.x() + kWorkAreaOffsetX,
            /*y=*/work_area.y() + work_area.height() / 2};
  bounds.set_origin(origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x(), bounds.x());
  EXPECT_EQ(origin.y(), bounds.y());

  // Position near the right edge.
  origin = {/*x=*/work_area.width() - kWorkAreaOffsetX,
            /*y=*/work_area.y() + work_area.height() / 2};
  bounds.set_origin(origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(work_area.x() + work_area.width() - bounds.width(), bounds.x());
  EXPECT_EQ(origin.y(), bounds.y());

  // Position near the top edge.
  origin = {/*x=*/work_area.x() + work_area.height() / 2,
            /*y=*/work_area.y() + kWorkAreaOffsetY};
  bounds.set_origin(origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(origin.x(), bounds.x());
  EXPECT_EQ(work_area.y(), bounds.y());

  // Position near the bottom edge.
  origin = {/*x=*/work_area.x() + work_area.height() / 2,
            /*y=*/work_area.height() - kWorkAreaOffsetY};
  bounds.set_origin(origin);
  SnapBoundsToEdgeOfWorkArea(work_area, &bounds);
  EXPECT_EQ(origin.x(), bounds.x());
  EXPECT_EQ(work_area.y() + work_area.height() - bounds.height(), bounds.y());
}

}  // namespace brave_ads
