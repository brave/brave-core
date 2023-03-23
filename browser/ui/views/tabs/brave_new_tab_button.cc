/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "brave/browser/ui/color/brave_color_id.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/layout_constants.h"
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

int BraveNewTabButton::GetChromiumOffsetForIcon() {
  return GetCornerRadius();
}

gfx::Size BraveNewTabButton::CalculatePreferredSize() const {
  // Overriden so that we use Brave's custom button size
  gfx::Size size = kButtonSize;
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        float scale,
                                        bool extend_to_top) const {
  return GetBorderPath(origin, scale, extend_to_top, GetCornerRadius(),
                       GetContentsBounds().size());
}

BraveNewTabButton::BraveNewTabButton(TabStrip* tab_strip,
                                     PressedCallback callback)
    : NewTabButton(tab_strip, std::move(callback)) {}

BraveNewTabButton::~BraveNewTabButton() = default;

void BraveNewTabButton::PaintIcon(gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped_canvas(canvas);
  // Incorrect offset that base class will use
  const int chromium_offset = GetChromiumOffsetForIcon();
  // Shim base implementation's painting
  // Overriden to fix chromium assumption that border radius
  // will be 50% of width.
  // Offset that we want to use
  const int correct_h_offset = (GetContentsBounds().width() / 2);

  // Difference
  const int h_offset = correct_h_offset - chromium_offset;
  canvas->Translate(gfx::Vector2d(h_offset, h_offset));

  NewTabButton::PaintIcon(canvas);
}

void BraveNewTabButton::PaintFill(gfx::Canvas* canvas) const {
  OnPaintFill(canvas);
}

void BraveNewTabButton::OnPaintFill(gfx::Canvas* canvas) const {
  NewTabButton::PaintFill(canvas);
}

gfx::Insets BraveNewTabButton::GetInsets() const {
  // Give an additional left margin to make more space from tab.
  // TabStripRegionView::UpdateNewTabButtonBorder() gives this button's inset.
  // So, adding more insets here is easy solution.
  return NewTabButton::GetInsets() + gfx::Insets::TLBR(0, 6, 0, 0);
}
