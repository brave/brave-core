/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <algorithm>

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/frame/browser_non_client_frame_view.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/scoped_canvas.h"

// static
const gfx::Size BraveNewTabButton::kButtonSize{24, 24};

// static
SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        float scale,
                                        bool extend_to_top,
                                        int corner_radius,
                                        const gfx::Size& contents_bounds) {
  // Overriden to use Brave's non-circular shape
  gfx::PointF scaled_origin(origin);
  scaled_origin.Scale(scale);
  const float radius = corner_radius * scale;

  SkPath path;
  const gfx::Rect path_rect(
      scaled_origin.x(), extend_to_top ? 0 : scaled_origin.y(),
      contents_bounds.width() * scale,
      (extend_to_top ? scaled_origin.y() : 0) +
          std::min(contents_bounds.width(), contents_bounds.height()) * scale);
  path.addRoundRect(RectToSkRect(path_rect), radius, radius);
  path.close();
  return path;
}

gfx::Size BraveNewTabButton::CalculatePreferredSize() const {
  // Overriden so that we use Brave's custom button size
  gfx::Size size = kButtonSize;
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser()))
    size.set_height(kHeightForVerticalTabs);

  return size;
}

SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        float scale,
                                        bool extend_to_top) const {
  return GetBorderPath(origin, scale, extend_to_top, GetCornerRadius(),
                       GetContentsBounds().size());
}

BraveNewTabButton::~BraveNewTabButton() = default;

void BraveNewTabButton::PaintIcon(gfx::Canvas* canvas) {
  // Overriden to fix chromium assumption that border radius
  // will be 50% of width.
  gfx::ScopedCanvas scoped_canvas(canvas);
  // Offset that we want to use
  const int correct_h_offset = (GetContentsBounds().width() / 2);
  // Incorrect offset that base class will use
  const int chromium_offset = GetCornerRadius();
  // Difference
  const int h_offset = correct_h_offset - chromium_offset;
  // Shim base implementation's painting
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    const int correct_v_offset = (GetContentsBounds().height() / 2);
    const int v_offset = correct_v_offset - chromium_offset;
    canvas->Translate(gfx::Vector2d(h_offset, v_offset));
  } else {
    canvas->Translate(gfx::Vector2d(h_offset, h_offset));
  }

  NewTabButton::PaintIcon(canvas);
}

gfx::Insets BraveNewTabButton::GetInsets() const {
  if (tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser()))
    return {};

  // Give an additional left margin to make more space from tab.
  // TabStripRegionView::UpdateNewTabButtonBorder() gives this button's inset.
  // So, adding more insets here is easy solution.
  return NewTabButton::GetInsets() + gfx::Insets::TLBR(0, 6, 0, 0);
}

void BraveNewTabButton::PaintFill(gfx::Canvas* canvas) const {
  if (!tabs::features::ShouldShowVerticalTabs(tab_strip_->GetBrowser())) {
    NewTabButton::PaintFill(canvas);
    return;
  }

  if (tab_strip_->GetCustomBackgroundId(BrowserFrameActiveState::kUseCurrent)
          .has_value()) {
    NewTabButton::PaintFill(canvas);
    return;
  }

  // Override stroke color
  gfx::ScopedCanvas scoped_canvas(canvas);
  canvas->UndoDeviceScaleFactor();
  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setColor(GetColorProvider()->GetColor(kColorToolbar));
  canvas->DrawPath(GetBorderPath(gfx::Point(), canvas->image_scale(), false),
                   flags);
}
