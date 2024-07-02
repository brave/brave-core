/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/tabs/switches.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "components/prefs/pref_service.h"

#if !BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/frame/window_frame_util.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/frame/opaque_browser_frame_view_layout.h"
#include "ui/base/theme_provider.h"
#include "ui/views/resources/grit/views_resources.h"
#endif

#if BUILDFLAG(IS_LINUX)
#include "brave/browser/ui/views/frame/brave_browser_frame_view_linux_native.h"
#include "chrome/browser/themes/theme_service.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/layout_constants.h"
#include "chrome/common/pref_names.h"
#include "ui/linux/linux_ui.h"
#include "ui/views/view_utils.h"
#include "ui/views/window/caption_button_layout_constants.h"
#include "ui/views/window/window_button_order_provider.h"
#endif

namespace tabs::utils {

bool SupportsVerticalTabs(const Browser* browser) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableVerticalTabsSwitch)) {
    return false;
  }

  if (!browser) {
    // During unit tests, |browser| can be null.
    CHECK_IS_TEST();
    return false;
  }

  return browser->is_type_normal();
}

bool ShouldShowVerticalTabs(const Browser* browser) {
  if (!SupportsVerticalTabs(browser)) {
    return false;
  }

  return browser->profile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsEnabled);
}

bool ShouldShowWindowTitleForVerticalTabs(const Browser* browser) {
  if (!ShouldShowVerticalTabs(browser)) {
    return false;
  }

  return browser->profile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow);
}

bool IsFloatingVerticalTabsEnabled(const Browser* browser) {
  if (!ShouldShowVerticalTabs(browser)) {
    return false;
  }

  return browser->profile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsFloatingEnabled);
}

bool IsVerticalTabOnRight(const Browser* browser) {
  return browser->profile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsOnRight);
}

std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserFrame* frame) {
#if BUILDFLAG(IS_MAC)
  // On Mac, window caption buttons are drawn by the system.
  return {80, 0};
#elif BUILDFLAG(IS_LINUX)
  if (!frame->UseCustomFrame()) {
    // We're using system provided title bar and border. As we don't have our
    // own window caption button at all, there's no caption button width.
    return {};
  }

  auto* browser_view =
      BrowserView::GetBrowserViewForNativeWindow(frame->GetNativeWindow());
  if (!browser_view) {
    // This can happen on startup
    return {};
  }

  auto* profile = browser_view->browser()->profile();
  auto* linux_ui_theme = ui::LinuxUiTheme::GetForProfile(profile);
  auto* theme_service_factory = ThemeServiceFactory::GetForProfile(profile);
  const bool using_gtk_caption_button =
      linux_ui_theme && theme_service_factory->UsingSystemTheme();
  if (!using_gtk_caption_button) {
    auto* window_order_provider =
        views::WindowButtonOrderProvider::GetInstance();
    return {views::GetCaptionButtonWidth() *
                window_order_provider->leading_buttons().size(),
            views::GetCaptionButtonWidth() *
                window_order_provider->trailing_buttons().size()};
  }

  // When using gtk-provided caption buttons, buttons' size and spacing is
  // decided by system. So we can't help but peeking the actual caption button's
  // position.
  auto* frame_view = views::AsViewClass<BraveBrowserFrameViewLinuxNative>(
      frame->GetFrameView());
  if (!frame_view) {
    // We could be in the middle of transition to GTK theme frame.
    return {};
  }
  return frame_view->leading_trailing_caption_button_width();

#elif BUILDFLAG(IS_WIN)
  if (frame->ShouldUseNativeFrame()) {
    // In this case, we use BrowserFrameViewWin. Native frame will be set to
    // the HWND and BrowserFrameViewWin will draw frame and window caption
    // button.
    auto size = WindowFrameUtil::GetWindowsCaptionButtonAreaSize();
    return {0, size.width()};
  }

  // In this case, we use OpaqueBrowserFrameView. OpaqueBrowserFrameView has
  // two types of frame button per platform but on Windows, it uses image
  // buttons. See OpaqueBrowserFrameView::GetFrameButtonStyle().
  int width = 0;
  // Uses image icons
  const ui::ThemeProvider* tp = frame->GetThemeProvider();
  DCHECK(tp);
  for (auto image_id : {IDR_MINIMIZE, IDR_MAXIMIZE, IDR_CLOSE}) {
    if (const gfx::ImageSkia* image = tp->GetImageSkiaNamed(image_id)) {
      width += image->width();
    }
  }
  return {0, width};
#else
#error "not handled platform"
#endif
}

}  // namespace tabs::utils
