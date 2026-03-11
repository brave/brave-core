// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/frame/layout/brave_browser_view_layout_delegate_impl.h"

#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/ui/views/frame/browser_frame_view.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

BrowserLayoutParams BraveBrowserViewLayoutDelegateImpl::GetBrowserLayoutParams(
    bool use_browser_bounds) const {
#if BUILDFLAG(IS_MAC)
  // On Mac, there is transition animation when entering/exiting fullscreen
  // mode. During the animation, there could be inconsistency between frame view
  // and browser view's state. Other platforms don't have this issue.
  if (use_browser_bounds) {
    const auto params = GetFrameView()->GetBrowserLayoutParams();
    if (params.IsEmpty()) {
      return params;
    }

    if (ShouldShowVerticalTabs() &&
        tabs::utils::ShouldShowWindowTitleForVerticalTabs(
            browser_view().browser()) &&
        !IsFullscreen() &&
        !params.visual_client_area.Contains(browser_view().bounds())) {
      // This could happen if the window is exiting fullscreen mode. The
      // browser wasn't laid out yet(insetted by the amount of the window
      // title bar), but is trying to layout its children from
      // BrowserView::Layout(). In this case, we should just return the empty
      // param and wait for the next layout triggered by frame view after it
      // repositioned the browser view.
      // https://github.com/brave/brave-browser/issues/53470
      return BrowserLayoutParams();
    }
  }
#endif  // BUILDFLAG(IS_MAC)
  return BrowserViewLayoutDelegateImpl::GetBrowserLayoutParams(
      use_browser_bounds);
}
