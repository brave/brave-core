/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/download/bubble/download_toolbar_button_view.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "ui/gfx/geometry/skia_conversions.h"

namespace gfx {

SkRect AdjustRingBounds(const gfx::RectF& ring_bounds);

}  // namespace gfx

#define RectFToSkRect(ring_bounds) AdjustRingBounds(ring_bounds)
#define DownloadToolbarButtonView DownloadToolbarButtonViewChromium

#include "src/chrome/browser/ui/views/download/bubble/download_toolbar_button_view.cc"

#undef DownloadToolbarButtonView
#undef RectFToSkRect

namespace gfx {

SkRect AdjustRingBounds(const gfx::RectF& ring_bounds) {
  const int chromium_ring_radius = ui::TouchUiController::Get()->touch_ui()
                                       ? kProgressRingRadiusTouchMode
                                       : kProgressRingRadius;
  constexpr auto kBraveRingRadius = 12;
  const auto delta = kBraveRingRadius - chromium_ring_radius;
  gfx::RectF new_bounds(ring_bounds);
  new_bounds.Outset(gfx::OutsetsF(delta));
  return gfx::RectFToSkRect(new_bounds);
}

}  // namespace gfx

SkColor DownloadToolbarButtonView::GetIconColor() const {
  const DownloadDisplayController::IconInfo icon_info = GetIconInfo();

  // Apply active color only when download is completed and user doesn't
  // interact with this button.
  if (icon_info.icon_state == download::DownloadIconState::kComplete &&
      icon_info.is_active) {
    return GetColorProvider()->GetColor(kColorBraveDownloadToolbarButtonActive);
  }

  // Otherwise, always use inactive color.
  return GetColorProvider()->GetColor(kColorDownloadToolbarButtonInactive);
}
