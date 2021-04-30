/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_BACKGROUND_PAINTER_H_
#define BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_BACKGROUND_PAINTER_H_

#include "ui/gfx/color_palette.h"
#include "ui/views/painter.h"

namespace brave_ads {

// Background Painter for ad notifications with rounded corners. This draws the
// rectangle with rounded corners
class AdNotificationBackgroundPainter : public views::Painter {
 public:
  AdNotificationBackgroundPainter(const int top_radius,
                                  const int bottom_radius,
                                  const SkColor color = SK_ColorWHITE);
  ~AdNotificationBackgroundPainter() override;

  // views::Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  const SkScalar top_radius_;
  const SkScalar bottom_radius_;
  const SkColor color_;

  AdNotificationBackgroundPainter(const AdNotificationBackgroundPainter&) =
      delete;
  AdNotificationBackgroundPainter& operator=(
      const AdNotificationBackgroundPainter&) = delete;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_BRAVE_ADS_AD_NOTIFICATION_BACKGROUND_PAINTER_H_
