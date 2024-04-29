/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"

#include <algorithm>
#include <utility>

#include "brave/browser/ui/tabs/features.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/compositor.h"
#include "ui/gfx/geometry/skia_conversions.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/gfx/scoped_canvas.h"

using tabs::features::HorizontalTabsUpdateEnabled;

namespace {

SkPath GetBorderPath(const gfx::Point& origin,
                     bool extend_to_top,
                     int corner_radius,
                     const gfx::Size& contents_bounds) {
  // Overridden to use Brave's non-circular shape
  SkPath path;
  const gfx::Rect path_rect(
      origin.x(), extend_to_top ? 0 : origin.y(), contents_bounds.width(),
      (extend_to_top ? origin.y() : 0) +
          std::min(contents_bounds.width(), contents_bounds.height()));
  path.addRoundRect(RectToSkRect(path_rect), corner_radius, corner_radius);
  path.close();
  return path;
}

}  // namespace

// static
gfx::Size BraveNewTabButton::GetButtonSize() {
  if (!HorizontalTabsUpdateEnabled()) {
    return {24, 24};
  }
  return {28, 28};
}

gfx::Size BraveNewTabButton::CalculatePreferredSize(
    const views::SizeBounds& available_size) const {
  // Overridden so that we use Brave's custom button size
  gfx::Size size = GetButtonSize();
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                        bool extend_to_top) const {
  if (GetWidget()) {
    return ::GetBorderPath(origin, extend_to_top, GetCornerRadius(),
                           GetContentsBounds().size());
  }
  return SkPath();
}

BraveNewTabButton::BraveNewTabButton(TabStrip* tab_strip,
                                     PressedCallback callback)
    : NewTabButton(tab_strip, std::move(callback)) {}

BraveNewTabButton::~BraveNewTabButton() = default;

void BraveNewTabButton::PaintIcon(gfx::Canvas* canvas) {
  gfx::ScopedCanvas scoped_canvas(canvas);

  if (HorizontalTabsUpdateEnabled()) {
    // Instead of letting `NewTabButton` draw a "plus", paint a vector icon to
    // the canvas in the center of the view.
    constexpr int kIconSize = 18;
    gfx::Rect bounds = GetContentsBounds();
    canvas->Translate(
        gfx::Vector2d((bounds.width() - kIconSize) / 2 + bounds.x(),
                      (bounds.height() - kIconSize) / 2 + bounds.y()));
    gfx::PaintVectorIcon(canvas, kLeoPlusAddIcon, kIconSize,
                         GetForegroundColor());
    return;
  }

  // Shim base implementation's painting
  // Overridden to fix chromium assumption that border radius
  // will be 50% of width.

  // Incorrect offset that base class will use
  const int chromium_offset = GetCornerRadius();

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

BEGIN_METADATA(BraveNewTabButton)
END_METADATA
