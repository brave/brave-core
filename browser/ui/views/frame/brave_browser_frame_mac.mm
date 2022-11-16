/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_mac.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

BraveBrowserFrameMac::BraveBrowserFrameMac(BrowserFrame* browser_frame,
                                           BrowserView* browser_view)
    : BrowserFrameMac(browser_frame, browser_view),
      browser_(browser_view->browser()),
      browser_view_(browser_view) {}

BraveBrowserFrameMac::~BraveBrowserFrameMac() = default;

void BraveBrowserFrameMac::GetWindowFrameTitlebarHeight(
    bool* override_titlebar_height,
    float* titlebar_height) {
  if (tabs::features::ShouldShowVerticalTabs(browser_)) {
    if (!tabs::features::ShouldShowWindowTitleForVerticalTabs(browser_)) {
      // In this case, titlbar height should be the same as toolbar height.
      *titlebar_height = browser_view_->toolbar()->GetPreferredSize().height();
      *override_titlebar_height = true;
      return;
    }

    // Otherwise, don't override titlebar height. The titlbar will be aligned
    // to the center of the given height automatically.
    return;
  }

  return BrowserFrameMac::GetWindowFrameTitlebarHeight(override_titlebar_height,
                                                       titlebar_height);
}
