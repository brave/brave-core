/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_NOTIFICATION_BACKGROUND_PAINTER_H_
#define BRAVE_UI_BRAVE_CUSTOM_NOTIFICATION_NOTIFICATION_BACKGROUND_PAINTER_H_

#include "brave/ui/brave_custom_notification/public/cpp/constants.h"
#include "ui/views/painter.h"

namespace brave_custom_notification {

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

}

#endif
