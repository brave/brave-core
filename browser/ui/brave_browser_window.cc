/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/brave_browser_window.h"

#include "chrome/browser/ui/views/frame/browser_view.h"

// static
BraveBrowserWindow* BraveBrowserWindow::From(BrowserWindow* window) {
  // BrowserView derives from BraveBrowserWindow, other BrowserWindow
  // implementations do not.
  return window ? window->AsBrowserView() : nullptr;
}

// static
BraveBrowserWindow* BraveBrowserWindow::FromBrowser(
    BrowserWindowInterface* browser) {
  return From(BrowserWindow::FromBrowser(browser));
}
