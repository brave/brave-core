/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_BACKGROUND_PAINTER_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_BACKGROUND_PAINTER_H_

#include "ui/views/painter.h"

namespace brave_ads {

// Background painter for notification ads with rounded corners. This draws the
// rectangle with rounded corners
class NotificationAdBackgroundPainter : public views::Painter {
 public:
  NotificationAdBackgroundPainter(const int top_radius,
                                  const int bottom_radius,
                                  const SkColor color = SK_ColorWHITE);

  NotificationAdBackgroundPainter(const NotificationAdBackgroundPainter&) =
      delete;
  NotificationAdBackgroundPainter& operator=(
      const NotificationAdBackgroundPainter&) = delete;

  NotificationAdBackgroundPainter(NotificationAdBackgroundPainter&&) noexcept =
      delete;
  NotificationAdBackgroundPainter& operator=(
      NotificationAdBackgroundPainter&&) noexcept = delete;

  ~NotificationAdBackgroundPainter() override;

  // views::Painter:
  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

 private:
  const SkScalar top_radius_;
  const SkScalar bottom_radius_;
  const SkColor color_;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_BACKGROUND_PAINTER_H_
