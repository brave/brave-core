/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/bounds_util.h"

#include "ui/display/screen.h"
#include "ui/gfx/geometry/point.h"
#include "ui/gfx/geometry/rect.h"

namespace brave_ads {

namespace {

enum class EdgeGravity { kTop, kBottom, kLeft, kRight };

gfx::Rect GetDisplayScreenWorkArea(gfx::Rect* bounds,
                                   gfx::NativeView native_view) {
  DCHECK(bounds);

  gfx::Rect work_area =
      display::Screen::GetScreen()->GetDisplayMatching(*bounds).work_area();

  if (work_area.IsEmpty()) {
    // There is no matching display for these bounds so we should move the ad
    // notification to the nearest display
    work_area = display::Screen::GetScreen()
                    ->GetDisplayNearestView(native_view)
                    .work_area();
  }

  return work_area;
}

void AdjustBoundsToFitWorkArea(const gfx::Rect& work_area, gfx::Rect* bounds) {
  DCHECK(bounds);

  bounds->AdjustToFit(work_area);
}

}  // namespace

void AdjustBoundsAndSnapToFitWorkAreaForNativeView(gfx::NativeView native_view,
                                                   gfx::Rect* bounds) {
  DCHECK(bounds);

  const gfx::Rect work_area = GetDisplayScreenWorkArea(bounds, native_view);
  AdjustBoundsToFitWorkArea(work_area, bounds);
  SnapBoundsToEdgeOfWorkArea(work_area, bounds);
}

void SnapBoundsToEdgeOfWorkArea(const gfx::Rect& work_area, gfx::Rect* bounds) {
  DCHECK(bounds);

  EdgeGravity gravity = EdgeGravity::kTop;
  int min_dist = bounds->y() - work_area.y();

  int dist =
      work_area.y() + work_area.height() - bounds->y() - bounds->height();
  if (min_dist > dist) {
    min_dist = dist;
    gravity = EdgeGravity::kBottom;
  }

  dist = bounds->x() - work_area.x();
  if (min_dist > dist) {
    min_dist = dist;
    gravity = EdgeGravity::kLeft;
  }

  dist = work_area.x() + work_area.width() - bounds->x() - bounds->width();
  if (min_dist > dist) {
    min_dist = dist;
    gravity = EdgeGravity::kRight;
  }

  switch (gravity) {
    case EdgeGravity::kTop:
      bounds->set_y(work_area.y());
      break;
    case EdgeGravity::kBottom:
      bounds->set_y(work_area.y() + work_area.height() - bounds->height());
      break;
    case EdgeGravity::kLeft:
      bounds->set_x(work_area.x());
      break;
    case EdgeGravity::kRight:
      bounds->set_x(work_area.x() + work_area.width() - bounds->width());
      break;
  }
}

}  // namespace brave_ads
