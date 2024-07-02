/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_popup_widget.h"

#include <utility>

#include "brave/components/brave_ads/browser/ad_units/notification_ad/custom_notification_ad_feature.h"
#include "build/build_config.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/widget/widget_delegate.h"

#if BUILDFLAG(IS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace brave_ads {

NotificationAdPopupWidget::NotificationAdPopupWidget() = default;

void NotificationAdPopupWidget::InitWidget(
    views::WidgetDelegate* delegate,
    const gfx::Rect& bounds,
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view) {
  CHECK(delegate);

  views::Widget::InitParams params(
      views::Widget::InitParams::TYPE_WINDOW_FRAMELESS);
  params.delegate = delegate;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  params.bounds = bounds;

  if constexpr (BUILDFLAG(IS_LINUX)) {
    // Chromium doesn't always support transparent window background on X11.
    // This can cause artifacts on shadows around ads notification popup. To fix
    // this shadows are drawn by Widget.
    params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;

    // TODO(https://github.com/brave/brave-browser/issues/29744): Enable
    // ZOrderLevel::kNormal for Linux when custom notification ad drawing
    // artifacts are fixed.
    params.z_order = ui::ZOrderLevel::kFloatingWindow;
    params.context = browser_native_window;
  } else {
    params.shadow_type = views::Widget::InitParams::ShadowType::kNone;

    if (kUseSameZOrderAsBrowserWindow.Get()) {
      params.z_order = ui::ZOrderLevel::kNormal;
      params.parent = browser_native_view;
    } else {
      params.z_order = ui::ZOrderLevel::kFloatingWindow;
      params.context = browser_native_window;
    }
  }

#if BUILDFLAG(IS_WIN)
  // We want to ensure that this toast always goes to the native desktop,
  // not the Ash desktop (since there is already another toast contents view
  // there
  if (!params.parent) {
    CHECK(!params.native_widget);
    params.native_widget = new views::DesktopNativeWidgetAura(this);
  }
#endif  // BUILDFLAG(IS_WIN)

  Init(std::move(params));
}

}  // namespace brave_ads
