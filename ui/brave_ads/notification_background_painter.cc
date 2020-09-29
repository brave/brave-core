// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "brave/ui/brave_ads/notification_background_painter.h"

#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/skia_util.h"

namespace brave_ads {

NotificationBackgroundPainter::NotificationBackgroundPainter(int top_radius,
                                                             int bottom_radius,
                                                             SkColor color)
    : top_radius_(SkIntToScalar(top_radius)),
      bottom_radius_(SkIntToScalar(bottom_radius)),
      color_(color) {}

NotificationBackgroundPainter::~NotificationBackgroundPainter() = default;

gfx::Size NotificationBackgroundPainter::GetMinimumSize() const {
  return gfx::Size();
}

void NotificationBackgroundPainter::Paint(gfx::Canvas* canvas,
                                          const gfx::Size& size) {
  SkPath path;
  SkScalar radii[8] = {top_radius_,    top_radius_,    top_radius_,
                       top_radius_,    bottom_radius_, bottom_radius_,
                       bottom_radius_, bottom_radius_};
  gfx::Rect rect(size);
  rect.Inset(insets_);
  path.addRoundRect(gfx::RectToSkRect(rect), radii);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(color_);
  canvas->DrawPath(path, flags);
}

}  // namespace brave_ads
