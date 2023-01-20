/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/features.h"

#include "base/check_is_test.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
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
#include "ui/views/window/caption_button_layout_constants.h"
#endif

namespace tabs {
namespace features {

BASE_FEATURE(kBraveVerticalTabs,
             "BraveVerticalTabs",
             base::FEATURE_DISABLED_BY_DEFAULT);

#if BUILDFLAG(IS_LINUX)
BASE_FEATURE(kBraveChangeActiveTabOnScrollEvent,
             "BraveChangeActiveTabOnScrollEvent",
             base::FEATURE_ENABLED_BY_DEFAULT);
#endif  // BUILDFLAG(IS_LINUX)

bool SupportsVerticalTabs(const Browser* browser) {
  DCHECK(base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
      << "Don't call this before checking the feature flag.";

  if (!browser) {
    // During unit tests, |browser| can be null.
    CHECK_IS_TEST();
    return false;
  }

  return browser->is_type_normal();
}

bool ShouldShowVerticalTabs(const Browser* browser) {
  if (!SupportsVerticalTabs(browser))
    return false;

  return browser->profile()->GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsEnabled);
}

bool ShouldShowWindowTitleForVerticalTabs(const Browser* browser) {
  if (!ShouldShowVerticalTabs(browser))
    return false;

  return browser->profile()->GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow);
}

bool IsFloatingVerticalTabsEnabled(const Browser* browser) {
  if (!ShouldShowVerticalTabs(browser))
    return false;

  return browser->profile()->GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsFloatingEnabled);
}

std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserFrame* frame) {
#if BUILDFLAG(IS_MAC)
  // On Mac, window caption buttons are drawn by the system.
  return {80, 0};
#elif BUILDFLAG(IS_LINUX)
  // On Linux, we can't overlay caption buttons on toolbar.
  return {0, 0};
#elif BUILDFLAG(IS_WIN)
  if (frame->ShouldUseNativeFrame()) {
    // In this case, we usd GlassBrowserFrameView. Native frame will be set to
    // the HWND and and GlassBrowserFrameView will draw frame and window caption
    // button.
    auto size = WindowFrameUtil::GetWindows10GlassCaptionButtonAreaSize();
    if (WindowFrameUtil::IsWin10TabSearchCaptionButtonEnabled(
            BrowserView::GetBrowserViewForNativeWindow(frame->GetNativeWindow())
                ->browser())) {
      size.set_width(
          size.width() + WindowFrameUtil::kWindows10GlassCaptionButtonWidth +
          WindowFrameUtil::kWindows10GlassCaptionButtonVisualSpacing);
    }
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
    if (const gfx::ImageSkia* image = tp->GetImageSkiaNamed(image_id))
      width += image->width();
  }
  return {0, width};
#else
#error "not handled platform"
#endif
}

}  // namespace features
}  // namespace tabs
