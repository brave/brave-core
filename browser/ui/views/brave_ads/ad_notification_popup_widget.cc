/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/ad_notification_popup_widget.h"

#include <utility>

#include "brave/components/brave_ads/common/features.h"
#include "build/build_config.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/widget/widget_delegate.h"

#if BUILDFLAG(IS_WIN)
#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"
#endif

namespace brave_ads {

AdNotificationPopupWidget::AdNotificationPopupWidget() = default;

void AdNotificationPopupWidget::InitWidget(
    views::WidgetDelegate* delegate,
    const gfx::Rect& bounds,
    gfx::NativeWindow browser_native_window,
    gfx::NativeView browser_native_view) {
  DCHECK(delegate);

  views::Widget::InitParams params;
  params.delegate = delegate;
  params.type = views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
  params.z_order = ui::ZOrderLevel::kFloatingWindow;
  params.opacity = views::Widget::InitParams::WindowOpacity::kTranslucent;
  // Chromium doesn't always support transparent window background on X11.
  // This can cause artifacts on shadows around ads notification popup. To fix
  // this shadows are drawn by Widget.
#if BUILDFLAG(IS_LINUX)
  params.shadow_type = views::Widget::InitParams::ShadowType::kDrop;
#else
  params.shadow_type = views::Widget::InitParams::ShadowType::kNone;
#endif  // BUILDFLAG(IS_LINUX)
  params.bounds = bounds;

  if (features::ShouldAttachAdNotificationToBrowserWindow()) {
    params.parent = browser_native_view;
  } else {
    params.context = browser_native_window;
  }

#if BUILDFLAG(IS_WIN)
  // We want to ensure that this toast always goes to the native desktop,
  // not the Ash desktop (since there is already another toast contents view
  // there
  if (!params.parent) {
    DCHECK(!params.native_widget);
    params.native_widget = new views::DesktopNativeWidgetAura(this);
  }
#endif  // BUILDFLAG(IS_WIN)

  Init(std::move(params));
}

}  // namespace brave_ads
