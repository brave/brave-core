/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_

#define set_desired_bounds_delegate                                 \
  set_desired_bounds_delegate_unused();                             \
  base::WeakPtr<WidgetDelegate> GetWeakPtr() {                      \
    return weak_ptr_factory_.GetWeakPtr();                          \
  }                                                                 \
  void set_desired_bounds_delegate

namespace brave_ads {
class NotificationAdPopup;
}  // namespace brave_ads

namespace brave_tooltips {
class BraveTooltipPopup;
}  // namespace brave_tooltips

class BraveOriginStartupView;
class MenuButtonDelegate;
class VerticalTabStripWidgetDelegateView;

#define CreatePassKey                                \
  CreatePassKey_Unused();                            \
  friend class ::brave_ads::NotificationAdPopup;     \
  friend class ::brave_tooltips::BraveTooltipPopup;  \
  friend class ::BraveOriginStartupView;             \
  friend class ::MenuButtonDelegate;                 \
  friend class ::VerticalTabStripWidgetDelegateView; \
  static WdvPassKey CreatePassKey

#include <ui/views/widget/widget_delegate.h>  // IWYU pragma: export

#undef set_desired_bounds_delegate
#undef CreatePassKey

#endif  // BRAVE_CHROMIUM_SRC_UI_VIEWS_WIDGET_WIDGET_DELEGATE_H_
