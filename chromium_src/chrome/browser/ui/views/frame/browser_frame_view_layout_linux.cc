/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/check_is_test.h"
#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "ui/views/window/caption_button_layout_constants.h"

#include "src/chrome/browser/ui/views/frame/browser_frame_view_layout_linux.cc"

int BrowserFrameViewLayoutLinux::NonClientTopHeight(bool restored) const {
  if (!view_) {
    CHECK_IS_TEST();
    return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
  }

  if (view_->browser_view()->browser()->is_type_normal() &&
      tabs::features::ShouldShowVerticalTabs()) {
    if (!view_->ShouldShowCaptionButtons()) {
      // In this case, the window manager might be forcibly providing system
      // window title or it's in fullscreen mode. We shouldn't show title bar
      // in this case.
      return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
    }

    if (tabs::features::ShouldShowWindowTitleForVerticalTabs(
            view_->browser_view()->browser())) {
      return 30;
    }

    // TODO(sko) For now, I couldn't find a way to overlay caption buttons
    // on toolbar. Once it gets possible, we shouldn't reserve non client top
    // height.
    if (view_->ShouldShowCaptionButtons()) {
      // Uses the same caption height defined in
      // c/b/u/v/frame/opaque_browser_frame_view_layout.cc
      return 18;
    }
  }

  return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
}
