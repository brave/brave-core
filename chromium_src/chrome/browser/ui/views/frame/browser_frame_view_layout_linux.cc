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
  if (!base::FeatureList::IsEnabled(tabs::features::kBraveVerticalTabs))
    return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);

  if (!view_) {
    CHECK_IS_TEST();
    return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
  }

  if (tabs::features::ShouldShowVerticalTabs(
          view_->browser_view()->browser())) {
    if (!view_->ShouldShowCaptionButtons()) {
      // In this case, the window manager might be forcibly providing system
      // window title or it's in fullscreen mode. We shouldn't show title bar
      // in this case.
      return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
    }

    // TODO(sko) For now, I couldn't find a way to overlay caption buttons
    // on toolbar. Once it gets possible, we shouldn't reserve non client top
    // height when window title isn't visible.

    // Adding 2px of vertical padding puts at least 1 px of space on the top and
    // bottom of the element.
    constexpr int kVerticalPadding = 2;
    // delegate_->GetIconSize() also considers the default font's height so
    // title will be visible.
    const int icon_height = FrameEdgeInsets(restored).top() +
                            delegate_->GetIconSize() + kVerticalPadding;

    const int caption_button_height =
        DefaultCaptionButtonY(restored) +
        18 /*kCaptionButtonHeight in OpaqueBrowserFrameView*/ +
        kCaptionButtonBottomPadding;
    return std::max(icon_height, caption_button_height) +
           kCaptionButtonBottomPadding;
  }

  return OpaqueBrowserFrameViewLayout::NonClientTopHeight(restored);
}
