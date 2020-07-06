// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_UI_BRAVE_ADS_NOTIFICATION_BACKGROUND_PAINTER_H_
#define BRAVE_UI_BRAVE_ADS_NOTIFICATION_BACKGROUND_PAINTER_H_

#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "ui/views/painter.h"

namespace brave_ads {

// Background Painter for notification. This is for notifications with rounded
// corners inside the unified message center. This draws the rectangle with
// rounded corners.
class NotificationBackgroundPainter
    : public views::Painter {
 public:
  NotificationBackgroundPainter(int top_radius,
                                int bottom_radius,
                                SkColor color = kNotificationBackgroundColor);
  ~NotificationBackgroundPainter() override;

  gfx::Size GetMinimumSize() const override;
  void Paint(gfx::Canvas* canvas, const gfx::Size& size) override;

  void set_insets(const gfx::Insets& insets) { insets_ = insets; }

 private:
  const SkScalar top_radius_;
  const SkScalar bottom_radius_;
  const SkColor color_;

  gfx::Insets insets_;

  DISALLOW_COPY_AND_ASSIGN(NotificationBackgroundPainter);
};

}  // namespace brave_ads

#endif  // BRAVE_UI_BRAVE_ADS_NOTIFICATION_BACKGROUND_PAINTER_H_
