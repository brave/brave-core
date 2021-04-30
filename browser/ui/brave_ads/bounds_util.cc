/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/bounds_util.h"

#include "ui/display/screen.h"
#include "ui/gfx/geometry/rect.h"

namespace brave_ads {

void AdjustBoundsToFitWorkAreaForNativeView(gfx::Rect* bounds,
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

  bounds->AdjustToFit(work_area);
}

}  // namespace brave_ads
