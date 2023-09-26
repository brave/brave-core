/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_window.h"

#include <vector>

// Provide a base implementation (important for `TestBrowserWindow ` in tests)
// For real implementation, see `BraveBrowserView`.

speedreader::SpeedreaderBubbleView* BraveBrowserWindow::ShowSpeedreaderBubble(
    speedreader::SpeedreaderTabHelper* tab_helper,
    speedreader::SpeedreaderBubbleLocation location) {
  return nullptr;
}

gfx::Rect BraveBrowserWindow::GetShieldsBubbleRect() {
  return gfx::Rect();
}

// static
BraveBrowserWindow* BraveBrowserWindow::From(BrowserWindow* window) {
  return static_cast<BraveBrowserWindow*>(window);
}

#if defined(TOOLKIT_VIEWS)
sidebar::Sidebar* BraveBrowserWindow::InitSidebar() {
  return nullptr;
}

void BraveBrowserWindow::ToggleSidebar() {}

bool BraveBrowserWindow::HasSelectedURL() const {
  return false;
}

void BraveBrowserWindow::CleanAndCopySelectedURL() {}

bool BraveBrowserWindow::ShowBraveHelpBubbleView(const std::string& text) {
  return false;
}

#endif
