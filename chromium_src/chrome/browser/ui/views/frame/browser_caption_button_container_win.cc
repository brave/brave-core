/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/frame/browser_caption_button_container_win.h"

#include "brave/browser/ui/views/frame/brave_browser_frame_view_win.h"
#include "chrome/browser/win/titlebar_config.h"
#include "ui/views/view_utils.h"

// In case vertical tab is visible without window title bar, we should show
// custom caption buttons over toolbar.
#define ShouldBrowserCustomDrawTitlebar(browser_view)        \
  (views::AsViewClass<BraveBrowserFrameViewWin>(frame_view_) \
       ->ShouldCaptionButtonsBeDrawnOverToolbar() ||         \
   ShouldBrowserCustomDrawTitlebar(browser_view))

#include <chrome/browser/ui/views/frame/browser_caption_button_container_win.cc>

#undef ShouldBrowserCustomDrawTitlebar
