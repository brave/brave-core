/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_root_view.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/ui/tabs/features.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/browser.h"

BraveBrowserRootView::BraveBrowserRootView(BrowserView* browser_view,
                                           views::Widget* widget)
    : BrowserRootView(browser_view, widget),
      browser_(browser_view->browser()->AsWeakPtr()) {
  if (!brave::IsRegularProfile(browser_->profile())) {
    theme_observation_.Observe(ui::NativeTheme::GetInstanceForNativeUi());
  }
}

BraveBrowserRootView::~BraveBrowserRootView() = default;

bool BraveBrowserRootView::OnMouseWheel(const ui::MouseWheelEvent& event) {
    // Bypass BrowserRootView::OnMouseWheel() to avoid tab cycling feature.
#if BUILDFLAG(IS_LINUX)
  if (!base::FeatureList::IsEnabled(
          tabs::features::kBraveChangeActiveTabOnScrollEvent)) {
    return RootView::OnMouseWheel(event);
  }
#endif

  // As vertical tabs are always in a scroll view, we should prefer scrolling
  // to tab cycling.
  if (tabs::utils::ShouldShowVerticalTabs(browser_.get())) {
    return RootView::OnMouseWheel(event);
  }

  return BrowserRootView::OnMouseWheel(event);
}

void BraveBrowserRootView::OnNativeThemeUpdated(
    ui::NativeTheme* observed_theme) {
  ThemeChanged();
}
