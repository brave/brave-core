/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/frame/brave_browser_frame_mac.h"

#include "brave/browser/ui/views/tabs/features.h"
#include "chrome/browser/ui/views/frame/browser_view.h"

BraveBrowserFrameMac::BraveBrowserFrameMac(BrowserFrame* browser_frame,
                                           BrowserView* browser_view)
    : BrowserFrameMac(browser_frame, browser_view),
      browser_(browser_view->browser()) {}

BraveBrowserFrameMac::~BraveBrowserFrameMac() = default;

void BraveBrowserFrameMac::GetWindowFrameTitlebarHeight(
    bool* override_titlebar_height,
    float* titlebar_height) {
  // Don't override titlebar height if the browser supports vertical tab strip 
  // so that it can overlay our client view. The visibility of title bar will be
  // controlled by BrowserNonClientFrameViewMac::UpdateWindowTitleVisibility.
  if (browser_->is_type_normal() && tabs::features::ShouldShowVerticalTabs())
    return;

  return BrowserFrameMac::GetWindowFrameTitlebarHeight(override_titlebar_height,
                                                       titlebar_height);
}
