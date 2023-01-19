/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_WIDGET_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_WIDGET_H_

#include "ui/gfx/native_widget_types.h"
#include "ui/views/widget/widget.h"

namespace gfx {
class Rect;
}  // namespace gfx

namespace views {
class WidgetDelegate;
}  // namespace views

namespace ui {
class NativeTheme;
}  // namespace ui

namespace brave_ads {

// Overrides base functionality of Widget to specify native theme used for ads
// notification popup.
class NotificationAdPopupWidget : public views::Widget {
 public:
  NotificationAdPopupWidget();

  void InitWidget(views::WidgetDelegate* delegate,
                  const gfx::Rect& bounds,
                  gfx::NativeWindow browser_native_window,
                  gfx::NativeView browser_native_view);
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_NOTIFICATION_AD_POPUP_WIDGET_H_
