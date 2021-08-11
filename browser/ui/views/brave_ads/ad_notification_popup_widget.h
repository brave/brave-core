/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_WIDGET_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_WIDGET_H_

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
class AdNotificationPopupWidget : public views::Widget {
 public:
  AdNotificationPopupWidget();

  void InitWidget(views::WidgetDelegate* delegate, const gfx::Rect& bounds);

  // views::Widget
  const ui::NativeTheme* GetNativeTheme() const override;
};

}  // namespace brave_ads

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_ADS_AD_NOTIFICATION_POPUP_WIDGET_H_
