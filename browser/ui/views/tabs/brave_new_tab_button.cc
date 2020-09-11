/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_new_tab_button.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/browser/ui/views/tabs/new_tab_button.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/scoped_canvas.h"
#include "ui/gfx/skia_util.h"

namespace {
  // Returns the size of the button without any margin
  gfx::Size GetButtonSize() {
    return gfx::Size(20, 20);
  }
}

// static
const gfx::Size BraveNewTabButton::kButtonSize{20, 20};

gfx::Size BraveNewTabButton::CalculatePreferredSize() const {
  // Overriden so that we use Brave's custom button size
  gfx::Size size = GetButtonSize();
  const auto insets = GetInsets();
  size.Enlarge(insets.width(), insets.height());
  return size;
}

SkPath BraveNewTabButton::GetBorderPath(const gfx::Point& origin,
                                      float scale,
                                      bool extend_to_top) const {
  // Overriden to use Brave's non-circular shape
  gfx::PointF scaled_origin(origin);
  scaled_origin.Scale(scale);
  const float radius = GetCornerRadius() * scale;

  SkPath path;
  const gfx::Rect contents_bounds = GetContentsBounds();
  const gfx::Rect path_rect(scaled_origin.x(),
              extend_to_top ? 0 : scaled_origin.y(),
              contents_bounds.width() * scale,
              scaled_origin.y() + contents_bounds.height() * scale);
  path.addRoundRect(RectToSkRect(path_rect), radius, radius);
  path.close();
  return path;
}

void BraveNewTabButton::PaintIcon(gfx::Canvas* canvas) {
  // Overriden to fix chromium assumption that border radius
  // will be 50% of width.
  gfx::ScopedCanvas scoped_canvas(canvas);
  // Offset that we want to use
  const int correct_offset = (GetContentsBounds().width() / 2);
  // Incorrect offset that base class will use
  const int chromium_offset = GetCornerRadius();
  // Difference
  const int offset = correct_offset - chromium_offset;
  // Shim base implementation's painting
  canvas->Translate(gfx::Vector2d(offset, offset));
  NewTabButton::PaintIcon(canvas);
}
