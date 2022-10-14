/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/features.h"

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

const base::Feature kBraveVerticalTabs{"BraveVerticalTabs",
                                       base::FEATURE_DISABLED_BY_DEFAULT};

bool ShouldShowVerticalTabs() {
  // TODO(sangwoo.ko) This should consider pref too.
  // https://github.com/brave/brave-browser/issues/23467
  return base::FeatureList::IsEnabled(features::kBraveVerticalTabs);
}

bool ShouldShowWindowTitleForVerticalTabs(const Browser* browser) {
  DCHECK(browser);

  if (!ShouldShowVerticalTabs())
    return false;

  if (!browser->is_type_normal())
    return false;

  return browser->profile()->GetOriginalProfile()->GetPrefs()->GetBoolean(
      brave_tabs::kVerticalTabsShowTitleOnWindow);
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
  // two types of frame button per platform but on Window, it uses image
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
