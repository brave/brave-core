/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_caption_button_container_win.h"

#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"
#include "brave/browser/ui/views/tabs/vertical_tab_utils.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/win/titlebar_config.h"
#include "ui/base/metadata/metadata_impl_macros.h"

#define BrowserCaptionButtonContainer BrowserCaptionButtonContainer_ChromiumImpl

// In case vertical tab is visible without window title bar, we should show
// custom caption buttons over toolbar.
#define ShouldBrowserCustomDrawTitlebar(browser_view)        \
  (static_cast<BraveBrowserFrameViewWin*>(frame_view_.get()) \
       ->ShouldCaptionButtonsBeDrawnOverToolbar() ||         \
   ShouldBrowserCustomDrawTitlebar(browser_view))

#include "src/chrome/browser/ui/views/frame/browser_caption_button_container_win.cc"

#undef ShouldBrowserCustomDrawTitlebar
#undef BrowserCaptionButtonContainer

BrowserCaptionButtonContainer::BrowserCaptionButtonContainer(
    BrowserFrameViewWin* frame_view)
    : BrowserCaptionButtonContainer_ChromiumImpl(frame_view),
      frame_view_(frame_view) {
}

BrowserCaptionButtonContainer::~BrowserCaptionButtonContainer() = default;

BEGIN_METADATA(BrowserCaptionButtonContainer)
END_METADATA
