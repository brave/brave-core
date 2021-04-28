/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_ads/ad_notification_background_painter.h"

#include "third_party/skia/include/core/SkPath.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/skia_util.h"

namespace brave_ads {

AdNotificationBackgroundPainter::AdNotificationBackgroundPainter(
    const int top_radius,
    const int bottom_radius,
    const SkColor color)
    : top_radius_(SkIntToScalar(top_radius)),
      bottom_radius_(SkIntToScalar(bottom_radius)),
      color_(color) {}

AdNotificationBackgroundPainter::~AdNotificationBackgroundPainter() = default;

gfx::Size AdNotificationBackgroundPainter::GetMinimumSize() const {
  return gfx::Size();
}

void AdNotificationBackgroundPainter::Paint(gfx::Canvas* canvas,
                                            const gfx::Size& size) {
  DCHECK(canvas);

  const gfx::Rect rect(size);

  SkScalar radii[8] = {top_radius_,    top_radius_,      // top-left
                       top_radius_,    top_radius_,      // top-right
                       bottom_radius_, bottom_radius_,   // bottom-right
                       bottom_radius_, bottom_radius_};  // bottom-left

  SkPath path;
  path.addRoundRect(gfx::RectToSkRect(rect), radii);

  cc::PaintFlags flags;
  flags.setAntiAlias(true);
  flags.setStyle(cc::PaintFlags::kFill_Style);
  flags.setColor(color_);

  canvas->DrawPath(path, flags);
}

}  // namespace brave_ads
