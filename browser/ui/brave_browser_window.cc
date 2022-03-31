/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_window.h"

// Provide a base implementation (important for `TestBrowserWindow ` in tests)
// For real implementation, see `BraveBrowserView`.

speedreader::SpeedreaderBubbleView* BraveBrowserWindow::ShowSpeedreaderBubble(
    speedreader::SpeedreaderTabHelper* tab_helper,
    bool is_enabled) {
  return nullptr;
}

#if BUILDFLAG(ENABLE_SIDEBAR)
sidebar::Sidebar* BraveBrowserWindow::InitSidebar() {
  return nullptr;
}
#endif
