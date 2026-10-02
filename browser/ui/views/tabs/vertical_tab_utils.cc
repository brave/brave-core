/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"

#include <string_view>

#include "base/numerics/safe_conversions.h"
#include "brave/browser/ui/focus_mode/focus_mode_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "brave/browser/ui/views/frame/brave_browser_view.h"
#include "build/build_config.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_widget.h"
#include "components/prefs/pref_service.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect_f.h"

namespace tabs::utils {

std::pair<int, int> GetLeadingTrailingCaptionButtonWidth(
    const BrowserWidget* frame) {
#if BUILDFLAG(IS_MAC)
  // On Mac, frame_view->GetBrowserLayoutParams() gives more wider width than
  // we want.
  return {80, 0};
#elif BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
  auto* frame_view = frame->GetFrameView();
  if (!frame_view) {
    return {};
  }
  const BrowserLayoutParams params = frame_view->GetBrowserLayoutParams();
  return {
      base::ClampCeil(params.leading_exclusion.ContentWithPadding().width()),
      base::ClampCeil(params.trailing_exclusion.ContentWithPadding().width())};
#else
#error "not handled platform"
#endif
}

bool IsVerticalTabToggleEnabled(BrowserWindowInterface* browser) {
#if BUILDFLAG(IS_MAC)
  if (!browser) {
    return true;
  }
  return chrome::BrowserCommandController::From(browser)->IsCommandEnabled(
      IDC_TOGGLE_VERTICAL_TABS);
#else
  return true;
#endif
}

}  // namespace tabs::utils

// Hooks declared (but not defined) in chromium_src overrides so that those
// overrides don't need to depend on Brave targets. Resolved at link time.
bool BraveShouldHideVerticalTabsCompletely(PrefService* prefs) {
  return prefs->FindPreference(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed) &&
         prefs->GetBoolean(
             brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed);
}

std::string_view BraveVerticalTabsHideCompletelyPrefName() {
  return brave_tabs::kVerticalTabsHideCompletelyWhenCollapsed;
}

bool BraveIsInVerticalTabHotCorner(const gfx::PointF& point_in_screen,
                                   BrowserView* browser_view) {
  auto* brave_browser_view = BraveBrowserView::From(browser_view);
  if (!brave_browser_view) {
    return false;
  }

  gfx::RectF hot_corner(
      brave_browser_view->GetBoundingBoxInScreenForMouseOverHandling());
  constexpr int kHotCornerWidth = 16;
  hot_corner.set_width(kHotCornerWidth);
  return hot_corner.Contains(point_in_screen);
}
